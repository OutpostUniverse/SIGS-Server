#include "Server.h"
#include "SocketInternal.h"
#include <algorithm>
#include <csignal>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace sigs::server
{
    static volatile std::sig_atomic_t running = 1;

    static void stop(int)
    {
        running = 0;
    }

    Server::Server(Config config)
        : config_(std::move(config)), accounts_(config_.accounts), storage_(config_.data),
          updates_(load_updates(config_.updates))
    {
        // Reject invalid configuration before opening endpoints that clients could start using.
        if (!config_.moderators.empty())
        {
            std::ifstream in(config_.moderators);
            std::string name;
            if (!in)
            {
                throw std::runtime_error("Cannot open moderator list");
            }
            while (in >> std::quoted(name))
            {
                if (!valid_name(name) || !accounts_.contains(name))
                {
                    throw std::runtime_error("Moderator must be an existing account");
                }
                moderators_.insert(key(name));
            }
            if (!in.eof())
            {
                throw std::runtime_error("Invalid moderator list");
            }
        }
        if (!config_.news.empty())
        {
            std::ifstream in(config_.news);
            std::string line;
            if (!in)
            {
                throw std::runtime_error("Cannot open news file");
            }
            while (std::getline(in, line))
            {
                if (line.empty() || line[0] == '#')
                {
                    continue;
                }
                std::istringstream row(line);
                int32_t revision;
                std::string category, text, extra;
                if (!(row >> revision >> std::quoted(category) >> std::quoted(text)) || row >> extra || revision <= 0 ||
                    category.empty() || category.size() > 64 || text.empty() || text.size() > 2000 ||
                    news_.size() >= 16)
                {
                    throw std::runtime_error("Invalid news record");
                }
                news_.push_back({revision, category, text});
            }
            if (!in.eof())
            {
                throw std::runtime_error("Cannot read news file");
            }
        }
        ipv4_address(config_.bind, 0);
        auto addr = ipv4_address(config_.advertise, 0);
        if (addr.sin_addr.s_addr == INADDR_ANY)
        {
            throw std::runtime_error("Advertise a reachable IPv4 address");
        }
        listen(Service::Validation, 0, config_.port);
        listen(Service::Lobby, 0, config_.port + 1);
        listen(Service::Room, 1, config_.port + 2);
        listen(Service::Control, 1, config_.port + 3);
        listen(Service::Data, 1, config_.port + 4);
        rooms_.emplace(1, Room{1, config_.port + 2, 50, "Main Lobby", "Main Lobby", {}});
        auto udp = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udp == INVALID_SOCKET)
        {
            throw std::runtime_error("Cannot create UDP probe socket");
        }
        probe_ = TcpSocket(static_cast<std::intptr_t>(udp));
        auto endpoint = ipv4_address(config_.bind, static_cast<uint16_t>(config_.port + 5));
        if (bind(udp, reinterpret_cast<sockaddr*>(&endpoint), sizeof(endpoint)))
        {
            throw std::runtime_error("Cannot bind UDP probe");
        }
        set_nonblocking(udp);
    }

    void Server::listen(Service service, int roomId, int port)
    {
        if (port < 1 || port > 65535)
        {
            throw std::runtime_error("Port out of range");
        }
        listeners_.push_back({service, roomId, TcpSocket::listen_on(config_.bind, static_cast<std::uint16_t>(port))});
    }

    void Server::send(Client& client, unsigned messageId, const Fields& fields)
    {
        if (client.closing)
        {
            return;
        }
        auto definition = FindMessageDefinition(SierraNW, messageId);
        if (!definition)
        {
            throw std::runtime_error("Unregistered outgoing message");
        }
        auto bytes = encode_packet({messageId, encode_fields(definition->format, fields)});
        // Bound slow-client memory without blocking the shared event loop on socket writes.
        if (client.queued + bytes.size() > 1024 * 1024)
        {
            client.closing = true;
            return;
        }
        client.queued += bytes.size();
        client.output.push_back(std::move(bytes));
    }

    void Server::broadcast(int roomId, unsigned messageId, const Fields& fields)
    {
        for (auto& [connectionId, client] : clients_)
        {
            if (client.service == Service::Room && client.room == roomId && !client.user.empty())
            {
                send(client, messageId, fields);
            }
        }
    }

    Client* Server::identify(const Client& client, const std::string& username)
    {
        // Legacy handoffs have no cryptographic token; matching peer IP cannot distinguish users behind shared NAT.
        for (auto& [connectionId, lobbyClient] : clients_)
        {
            if (lobbyClient.service == Service::Lobby && !lobbyClient.closing && !lobbyClient.user.empty() &&
                lobbyClient.ip == client.ip && key(lobbyClient.user) == key(username))
            {
                return &lobbyClient;
            }
        }
        return nullptr;
    }

    Client* Server::player(const std::string& username, int roomId)
    {
        for (auto& [connectionId, client] : clients_)
        {
            if (!client.closing && client.service == Service::Room && client.room == roomId &&
                key(client.user) == key(username))
            {
                return &client;
            }
        }
        return nullptr;
    }

    Client* Server::channel(const Client& client, Service service)
    {
        // Iteration order chooses the newest live role connection when a client reconnects during setup.
        Client* latestConnection = nullptr;
        for (auto& [connectionId, otherClient] : clients_)
        {
            if (!otherClient.closing && otherClient.service == service && otherClient.room == client.room &&
                otherClient.owner == client.owner && !otherClient.user.empty())
            {
                latestConnection = &otherClient;
            }
        }
        return latestConnection;
    }

    Game* Server::game(unsigned playerId, int roomId)
    {
        for (auto& [gameName, currentGame] : rooms_.at(roomId).games)
        {
            if (currentGame.slots.count(playerId))
            {
                return &currentGame;
            }
        }
        return nullptr;
    }

    void Server::population(int roomId)
    {
        int populationCount = 0;
        for (auto& [connectionId, client] : clients_)
        {
            if (client.service == Service::Room && client.room == roomId && !client.user.empty() && !client.closing)
            {
                ++populationCount;
            }
        }
        for (auto& [connectionId, client] : clients_)
        {
            if (client.service == Service::Lobby && !client.user.empty())
            {
                send(client, 310, {int32_t(roomId), int32_t(populationCount)});
            }
        }
    }

    void Server::listing(Client& client, const Room& roomEntry)
    {
        send(client, 306,
             {roomEntry.name, std::string("OUTPOST2"), int32_t(roomEntry.id), int32_t(0), int32_t(0), int32_t(0),
              uint8_t(0), roomEntry.password, roomEntry.description, config_.advertise, int32_t(roomEntry.port)});
    }

    Fields Server::record_for(const Client& subject, const Client& recipient) const
    {
        auto record = subject.record;
        // shortcut: equal public IPs imply a shared LAN; revisit if CGNAT peers need distinct external mappings.
        auto address = ipv4_address(subject.ip, 0);
        auto hostAddress = ntohl(address.sin_addr.s_addr);
        auto firstOctet = hostAddress >> 24, secondOctet = (hostAddress >> 16) & 255;
        bool public_ip = firstOctet != 0 && firstOctet != 10 && firstOctet != 127 && firstOctet < 224 &&
                         !(firstOctet == 192 && secondOctet == 168) &&
                         !(firstOctet == 172 && secondOctet >= 16 && secondOctet <= 31) &&
                         !(firstOctet == 169 && secondOctet == 254) &&
                         !(firstOctet == 100 && secondOctet >= 64 && secondOctet <= 127);
        if (subject.ip != recipient.ip && public_ip)
        {
            record.at(5) = subject.ip;
        }
        return record;
    }

    void Server::refresh(Client& subject)
    {
        for (auto& [connectionId, client] : clients_)
        {
            if (!client.user.empty() && client.service == Service::Room && client.room == subject.room)
            {
                send(client, 111, record_for(subject, client));
            }
        }
    }

    void Server::disconnect(Client& client)
    {
        if (client.service == Service::Lobby)
        {
            for (auto& [connectionId, otherClient] : clients_)
            {
                if (otherClient.owner == client.id && connectionId != client.id)
                {
                    otherClient.closing = true;
                }
            }
        }
        if (client.service == Service::Room && !client.user.empty())
        {
            leave(client);
            broadcast(client.room, 202, {client.user});
            for (auto& [connectionId, otherClient] : clients_)
            {
                if (otherClient.owner == client.owner && otherClient.room == client.room &&
                    (otherClient.service == Service::Control || otherClient.service == Service::Data))
                {
                    otherClient.closing = true;
                }
            }
            pending_.erase(client.id);
            population(client.room);
        }
        if ((client.service == Service::Data || client.service == Service::Control) && !client.user.empty())
        {
            // Closing a superseded socket must not clear the replacement connection's readiness.
            if (auto otherClient = channel(client, client.service))
            {
                if (otherClient->id != client.id)
                {
                    return;
                }
            }
            if (auto controlConnection = channel(client, Service::Control))
            {
                controlConnection->ready = false;
            }
        }
    }

    void Server::handle(Client& client, const Packet& packet, unsigned depth, unsigned* budget)
    {
        if (client.closing)
        {
            return;
        }
        unsigned remaining = 128;
        // Share one budget through the entire nested tree so siblings cannot each reset the work limit.
        if (!budget)
        {
            budget = &remaining;
        }
        if (!*budget || depth > 4)
        {
            throw std::runtime_error("Compound message limit");
        }
        --*budget;
        if (packet.id == 1)
        {
            auto fields = decode_fields("%b", packet.payload);
            const auto& block = std::get<Bytes>(fields[0]);
            size_t offset = 0;
            while (offset < block.size())
            {
                if (block.size() - offset < 12)
                {
                    throw std::runtime_error("Truncated compound header");
                }
                auto length = read_be32(block, offset);
                if (length < 12 || length > block.size() - offset)
                {
                    throw std::runtime_error("Invalid compound message size");
                }
                Bytes frame;
                append_be32(frame, length);
                // Container children omit the TCP outer length; restore it to reuse the strict frame decoder.
                frame.insert(frame.end(), block.begin() + offset, block.begin() + offset + length);
                handle(client, decode_packet(frame), depth + 1, budget);
                offset += length;
            }
            return;
        }
        if (packet.payload.size() > 65536)
        {
            throw std::runtime_error("Message too large");
        }
        auto definition = FindMessageDefinition(SierraNW, packet.id);
        if (!definition)
        {
            if (client.unsupported.size() < 128 && client.unsupported.insert(packet.id).second)
            {
                std::cerr << "Unknown message id=" << packet.id << " connection=" << client.id
                          << " bytes=" << packet.payload.size() << '\n';
            }
            return;
        }
        auto fields = decode_fields(definition->format, packet.payload);
        for (const auto& value : fields)
        {
            if (auto stringValue = std::get_if<std::string>(&value); stringValue && stringValue->size() > 4096)
            {
                throw std::runtime_error("Field too large");
            }
        }
        if (std::string(message_support(packet.id)) != "handled")
        {
            // Even unresolved fields may contain passwords/private text; log metadata only and invent no reply.
            if (client.unsupported.insert(packet.id).second)
            {
                std::cerr << "Unsupported incoming id=" << packet.id << " status=" << message_support(packet.id)
                          << " connection=" << client.id << " bytes=" << packet.payload.size() << '\n';
            }
            return;
        }
        client.last = Clock::now();
        client.pinged = false; // Partial bytes and unsupported packets do not reset the watchdog.
        switch (client.service)
        {
        case Service::Validation:
            validation(client, packet.id, fields);
            break;
        case Service::Lobby:
            lobby(client, packet.id, fields);
            break;
        case Service::Room:
            room(client, packet.id, fields);
            break;
        default:
            transport(client, packet.id, fields);
            break;
        }
    }

    void Server::pump_update(Client& client)
    {
        // Refill incrementally: a valid package can exceed the per-client output cap by many megabytes.
        if (client.closing || client.validation_stage != 4 || client.update < 0 || client.queued >= 32768)
        {
            return;
        }
        const auto& package = updates_.at(static_cast<size_t>(client.update));
        while (client.update_offset < package.bytes.size() && client.queued < 32768)
        {
            auto blockSize = std::min<size_t>(512, package.bytes.size() - client.update_offset);
            auto blockBegin = package.bytes.begin() + client.update_offset;
            send(client, 905, {Bytes(blockBegin, blockBegin + blockSize)});
            client.update_offset += blockSize;
        }
        if (client.update_offset == package.bytes.size())
        {
            send(client, 906);
            // Component updates continue to the lobby endpoint; game updates end the validation connection.
            if (package.kind == "sigs")
            {
                send(client, 901, {config_.advertise, int32_t(config_.port + 1)});
                client.validation_stage = 1;
            }
            else
            {
                client.validation_stage = 5;
                client.closing = true;
            }
            client.update = -1;
        }
    }

    void Server::retire_rooms()
    {
        for (auto roomIterator = rooms_.begin(); roomIterator != rooms_.end();)
        {
            auto& room = roomIterator->second;
            bool occupied = false;
            for (auto& [connectionId, client] : clients_)
            {
                if (client.service == Service::Room && client.room == room.id && !client.user.empty())
                {
                    occupied = true;
                }
            }
            if (room.id == 1 || !room.used || occupied)
            {
                ++roomIterator;
                continue;
            }
            population(room.id);
            for (auto& [connectionId, client] : clients_)
            {
                if (client.service == Service::Lobby && !client.user.empty())
                {
                    // A 309 naming the active room makes the original client exit; defer it for affected owners.
                    if (client.room_risks.count(room.id))
                    {
                        client.removals[room.id] = room.name;
                    }
                    else
                    {
                        send(client, 309, {room.name});
                    }
                    if (client.admitted == room.id)
                    {
                        client.admitted = 0;
                    }
                }
                if (client.room == room.id && client.service != Service::Lobby)
                {
                    client.closing = true;
                }
            }
            auto retiredRoomId = room.id;
            listeners_.erase(std::remove_if(listeners_.begin(), listeners_.end(),
                                            [=](const Listener& listener)
                                            {
                                                return listener.room == retiredRoomId;
                                            }),
                             listeners_.end());
            roomIterator = rooms_.erase(roomIterator);
        }
    }

    void Server::probe()
    {
        char buffer[513];
        sockaddr_in senderAddress{};
        int addressLength = sizeof(senderAddress);
        auto probeSocket = native(probe_.native_handle());
        auto bytesReceived = recvfrom(probeSocket, buffer, sizeof(buffer), 0,
                                      reinterpret_cast<sockaddr*>(&senderAddress), &addressLength);
        if (bytesReceived < 12 || bytesReceived > 512)
        {
            return;
        }
        try
        {
            Bytes wire;
            append_be32(wire, static_cast<uint32_t>(bytesReceived));
            // UDP carries only the inner header; the temporary outer word lets the TCP codec validate it too.
            wire.insert(wire.end(), buffer, buffer + bytesReceived);
            auto packet = decode_packet(wire);
            if (packet.id != 102)
            {
                return;
            }
            auto fields = decode_fields("%s %s %i", packet.payload);
            char sourceIp[INET_ADDRSTRLEN]{};
            if (!inet_ntop(AF_INET, &senderAddress.sin_addr, sourceIp, sizeof(sourceIp)))
            {
                return;
            }
            for (auto& [connectionId, client] : clients_)
            {
                if (client.service != Service::Room || client.closing ||
                    key(client.user) != key(std::get<std::string>(fields[0])) || client.ip != sourceIp ||
                    Clock::now() > client.probe_expires || client.probe_replies >= 3)
                {
                    continue;
                }
                ++client.probe_replies;
                // Reply only to the observed source, never the untrusted address/port inside message 102.
                sendto(probeSocket, buffer, bytesReceived, 0, reinterpret_cast<sockaddr*>(&senderAddress),
                       addressLength);
                return;
            }
        }
        catch (const std::exception&)
        { /* Ignore malformed datagrams. */
        }
    }

    void Server::run()
    {
        std::signal(SIGINT, stop);
        std::signal(SIGTERM, stop);
        std::cout << "SIGS 3.0.3.0 server: " << config_.bind << ':' << config_.port << " through " << config_.port + 4
                  << std::endl;
        while (running)
        {
            fd_set reads, writes;
            FD_ZERO(&reads);
            FD_ZERO(&writes);
            FD_SET(native(probe_.native_handle()), &reads);
            for (auto& listener : listeners_)
            {
                FD_SET(native(listener.socket.native_handle()), &reads);
            }
            for (auto& [connectionId, client] : clients_)
            {
                auto descriptor = native(client.socket.native_handle());
                if (!client.closing)
                {
                    FD_SET(descriptor, &reads);
                }
                if (!client.output.empty())
                {
                    FD_SET(descriptor, &writes);
                }
            }
            timeval timeout{0, 100000};
            if (select(0, &reads, &writes, nullptr, &timeout) == SOCKET_ERROR)
            {
                throw std::runtime_error("select failed");
            }
            if (FD_ISSET(native(probe_.native_handle()), &reads))
            {
                probe();
            }
            for (auto& listener : listeners_)
            {
                if (FD_ISSET(native(listener.socket.native_handle()), &reads))
                {
                    auto connectionSocket = listener.socket.accept_one(100);
                    if (clients_.size() >= 128)
                    {
                        continue;
                    }
                    sockaddr_in peer{};
                    int length = sizeof(peer);
                    if (getpeername(native(connectionSocket.native_handle()), reinterpret_cast<sockaddr*>(&peer),
                                    &length))
                    {
                        continue;
                    }
                    char address[INET_ADDRSTRLEN]{};
                    if (!inet_ntop(AF_INET, &peer.sin_addr, address, sizeof(address)))
                    {
                        continue;
                    }
                    auto connectionId = next_client_++;
                    Client client{};
                    client.id = connectionId;
                    client.service = listener.service;
                    client.room = listener.room;
                    client.socket = std::move(connectionSocket);
                    client.ip = address;
                    clients_.emplace(connectionId, std::move(client));
                }
            }
            for (auto& [connectionId, client] : clients_)
            {
                auto connectionSocket = native(client.socket.native_handle());
                try
                {
                    if (!client.closing && FD_ISSET(connectionSocket, &reads))
                    {
                        auto bytes = client.socket.receive(65536, 100);
                        if (bytes.empty())
                        {
                            client.frames.finish();
                            client.closing = true;
                        }
                        else
                        {
                            client.frames.feed(bytes,
                                               [&](const Packet& p)
                                               {
                                                   handle(client, p);
                                               });
                        }
                    }
                    if (FD_ISSET(connectionSocket, &writes) && !client.output.empty())
                    {
                        // Nonblocking send may consume only part of a frame; keep its remaining bytes queued.
                        auto& bytes = client.output.front();
                        auto bytesSent =
                            ::send(connectionSocket, reinterpret_cast<const char*>(bytes.data() + client.offset),
                                   static_cast<int>(bytes.size() - client.offset), 0);
                        if (bytesSent > 0)
                        {
                            client.offset += bytesSent;
                            client.queued -= bytesSent;
                            if (client.service == Service::Validation &&
                                (client.validation_stage == 4 || client.validation_stage == 5))
                            {
                                client.last = Clock::now();
                            }
                            if (client.offset == bytes.size())
                            {
                                client.output.pop_front();
                                client.offset = 0;
                            }
                        }
                        else if (bytesSent == 0 || !socket_would_block())
                        {
                            throw std::runtime_error("Write failed");
                        }
                    }
                    pump_update(client);
                    auto idle = Clock::now() - client.last;
                    // Heartbeat intervals are emulator policy; no application ping is invented for the lobby.
                    bool monitored = !client.user.empty() && client.service != Service::Lobby;
                    if ((client.user.empty() && idle > std::chrono::seconds(30)) ||
                        (monitored && idle > std::chrono::seconds(90)))
                    {
                        client.closing = true;
                    }
                    if (monitored && idle > std::chrono::seconds(30) && !client.pinged)
                    {
                        send(client, 207);
                        client.pinged = true;
                    }
                    if (client.closing && idle > std::chrono::seconds(90))
                    {
                        client.output.clear();
                    }
                }
                catch (const std::exception&)
                {
                    // Isolate malformed traffic and storage/transport failures to this connection without exposing
                    // fields.
                    std::cerr << "Closing connection " << connectionId << " after a protocol/transport error\n";
                    client.closing = true;
                    client.output.clear();
                }
            }
            for (auto connectionIterator = pending_.begin(); connectionIterator != pending_.end();)
            {
                if (Clock::now() < connectionIterator->second.expires)
                {
                    ++connectionIterator;
                    continue;
                }
                auto client = clients_.find(connectionIterator->first);
                if (client != clients_.end() && !connectionIterator->second.invite)
                {
                    send(client->second, 410, {int32_t(-1)});
                }
                if (client != clients_.end() && connectionIterator->second.invite)
                {
                    auto host = clients_.find(connectionIterator->second.host);
                    if (host != clients_.end())
                    {
                        send(host->second, 408, {int32_t(0), client->second.user});
                    }
                    for (auto service : {Service::Control, Service::Data})
                    {
                        if (auto transport = channel(client->second, service))
                        {
                            transport->closing = true;
                        }
                    }
                }
                connectionIterator = pending_.erase(connectionIterator);
            }
            // Cleanup can notify other clients; defer erasure until every readiness result has been processed.
            for (auto connectionIterator = clients_.begin(); connectionIterator != clients_.end();)
            {
                if (!connectionIterator->second.closing || !connectionIterator->second.output.empty())
                {
                    ++connectionIterator;
                    continue;
                }
                disconnect(connectionIterator->second);
                connectionIterator = clients_.erase(connectionIterator);
            }
            retire_rooms();
        }
    }
} // namespace sigs::server
