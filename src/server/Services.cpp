#include "Server.h"
#include <algorithm>

namespace sigs::server
{
    static bool valid_display_name(const std::string& name)
    {
        if (name.empty() || name.size() > 32 || name.find_first_not_of(' ') == std::string::npos)
        {
            return false;
        }
        // Room and game labels may contain spaces, unlike account identifiers.
        return std::all_of(name.begin(), name.end(),
                           [](unsigned char character)
                           {
                               return character >= 32 && character <= 126 && character != '"' && character != '\\';
                           });
    }

    static const std::string& stringField(const Fields& fields, size_t fieldIndex)
    {
        return std::get<std::string>(fields.at(fieldIndex));
    }

    static int32_t integerField(const Fields& fields, size_t fieldIndex)
    {
        return std::get<int32_t>(fields.at(fieldIndex));
    }

    void Server::validation(Client& client, unsigned messageId, const Fields& fields)
    {
        if (messageId == 617)
        {
            // The offer is connection-scoped; a bare download request must never select a package.
            if (client.update < 0 || (client.validation_stage != 2 && client.validation_stage != 3))
            {
                throw std::runtime_error("Download requires a pending offer");
            }
            client.validation_stage = 4;
            client.update_offset = 0;
            send(client, 904, {int32_t(updates_.at(client.update).bytes.size())});
            pump_update(client);
            return;
        }
        if (messageId == 616 && client.validation_stage == 0)
        {
            for (size_t updateIndex = 0; updateIndex < updates_.size(); ++updateIndex)
            {
                auto& update = updates_[updateIndex];
                if (update.kind == "sigs" && key(update.identifier) == key(stringField(fields, 0)) &&
                    update.from == stringField(fields, 1))
                {
                    client.update = static_cast<int>(updateIndex);
                    client.validation_stage = 2;
                    send(client, 903, {update.target, int32_t(update.bytes.size())});
                    return;
                }
            }
            // Other profiles need different layouts; only an explicit update route accepts another version.
            if (stringField(fields, 1) != "3.0.3.0")
            {
                client.closing = true;
                return;
            }
            client.validation_stage = 1;
            send(client, 901, {config_.advertise, int32_t(config_.port + 1)});
        }
        else if (messageId == 626 && client.validation_stage == 1)
        {
            for (size_t updateIndex = 0; updateIndex < updates_.size(); ++updateIndex)
            {
                auto& update = updates_[updateIndex];
                if (update.kind == "game" && key(update.identifier) == key(stringField(fields, 0)) &&
                    update.from == stringField(fields, 1))
                {
                    client.update = static_cast<int>(updateIndex);
                    client.validation_stage = 3;
                    send(client, 907, {int32_t(-1), int32_t(update.bytes.size()), uint16_t(update.required)});
                    return;
                }
            }
            if (key(stringField(fields, 0)) == "outpost2")
            {
                send(client, 907, {int32_t(0), int32_t(0), uint16_t(0)});
            }
            else
            {
                send(client, 902);
            }
            client.closing = true;
        }
        else
        {
            throw std::runtime_error("Invalid validation sequence");
        }
    }

    void Server::lobby(Client& client, unsigned messageId, const Fields& fields)
    {
        if (messageId == 601 && client.user.empty())
        {
            if (++client.attempts > 5)
            {
                client.closing = true;
                return;
            }
            auto now = Clock::now();
            if (now - client.login_attempt < std::chrono::seconds(1))
            {
                send(client, 311, {int32_t(-4), std::string()});
                return;
            }
            client.login_attempt = now;
            bool accepted = valid_name(stringField(fields, 0)) &&
                            (config_.permissive || accounts_.verify(stringField(fields, 0), stringField(fields, 1)));
            for (const auto& [entryId, other] : clients_)
            {
                if (!other.closing && other.service == Service::Lobby && !other.user.empty() &&
                    key(other.user) == key(stringField(fields, 0)))
                {
                    accepted = false;
                }
            }
            if (!accepted)
            {
                send(client, 311, {int32_t(-4), std::string()});
                return;
            }
            client.user = stringField(fields, 0);
            client.owner = client.id;
            send(client, 311, {int32_t(0), client.user});
            send(client, 314, storage_.membership(client.user));
            for (auto& [entryId, roomEntry] : rooms_)
            {
                listing(client, roomEntry);
                population(entryId);
            }
            for (auto& news : news_)
            {
                send(client, 803, news);
            }
            return;
        }
        if (messageId == 620)
        {
            // 301's recovered payload is a float; no mapping from dotted SIGS versions is assumed.
            if (config_.required_dll > 0)
            {
                send(client, 301, {config_.required_dll});
                return;
            }
            if (!config_.signup || !client.user.empty() || ++client.attempts > 3 ||
                !valid_name(stringField(fields, 0)) || accounts_.contains(stringField(fields, 0)))
            {
                send(client, 302,
                     {int32_t(-1), std::string(),
                      std::string("Registration unavailable or member name already exists")});
                return;
            }
            try
            {
                // The original client supplies a placeholder, not a usable account password.
                storage_.save_membership(stringField(fields, 0), fields, 620);
                Accounts::add(config_.accounts, stringField(fields, 0), random_secret());
                accounts_ = Accounts(config_.accounts);
                send(client, 302,
                     {int32_t(0), stringField(fields, 0),
                      std::string(
                          "Account registered. Contact the administrator to set your password; no email was sent.")});
            }
            catch (const std::exception&)
            {
                send(client, 302, {int32_t(-1), std::string(), std::string("Registration could not be saved")});
            }
            return;
        }
        if (client.user.empty())
        {
            throw std::runtime_error("Login required");
        }
        if (messageId == 602)
        {
            auto roomIterator = rooms_.find(integerField(fields, 0));
            int count = 0;
            for (auto& [entryId, roomPlayer] : clients_)
            {
                if (!roomPlayer.closing && roomPlayer.service == Service::Room &&
                    roomPlayer.room == integerField(fields, 0) && !roomPlayer.user.empty())
                {
                    ++count;
                }
            }
            bool allowed = roomIterator != rooms_.end() && count < roomIterator->second.capacity;
            client.admitted = allowed ? roomIterator->first : 0;
            // The next 111 must consume this grant promptly on the same authenticated lobby owner's behalf.
            client.probe_expires = allowed ? Clock::now() + std::chrono::seconds(30) : Clock::time_point{};
            send(client, 303, {int32_t(allowed ? 0 : roomIterator != rooms_.end() ? -2 : -1)});
        }
        else if (messageId == 603)
        {
            bool valid = valid_display_name(stringField(fields, 0)) && stringField(fields, 1).size() <= 256 &&
                         key(stringField(fields, 2)) == "outpost2" && integerField(fields, 3) >= 2 &&
                         integerField(fields, 3) <= 50 && std::get<uint8_t>(fields.at(4)) <= 1 &&
                         stringField(fields, 5).size() <= 128 && rooms_.size() < 16;
            for (auto& [entryId, roomEntry] : rooms_)
            {
                if (key(roomEntry.name) == key(stringField(fields, 0)))
                {
                    valid = false;
                }
            }
            // 309 deletes by name, so reusing a name before deferred removals arrive could delete the new listing.
            for (auto& [connectionId, lobbyClient] : clients_)
            {
                for (auto& [removedRoomId, roomName] : lobbyClient.removals)
                {
                    if (key(roomName) == key(stringField(fields, 0)))
                    {
                        valid = false;
                    }
                }
            }
            if (!valid)
            {
                send(client, 304, {int32_t(-1)});
                return;
            }
            int roomId = next_room_;
            int port = config_.port + 10 + (roomId - 2) * 3;
            auto previousListenerCount = listeners_.size();
            // Publish the directory entry only after its full room/control/data listener triple has bound.
            try
            {
                listen(Service::Room, roomId, port);
                listen(Service::Control, roomId, port + 1);
                listen(Service::Data, roomId, port + 2);
            }
            catch (...)
            {
                listeners_.resize(previousListenerCount);
                send(client, 304, {int32_t(-1)});
                return;
            }
            ++next_room_;
            auto& roomEntry =
                rooms_
                    .emplace(
                        roomId,
                        Room{roomId, port, integerField(fields, 3), stringField(fields, 0), stringField(fields, 1), {}})
                    .first->second;
            roomEntry.password = stringField(fields, 5);
            send(client, 304, {int32_t(0)});
            for (auto& [connectionId, lobbyClient] : clients_)
            {
                if (lobbyClient.service == Service::Lobby && !lobbyClient.user.empty())
                {
                    listing(lobbyClient, roomEntry);
                }
            }
        }
        else if (messageId == 612)
        {
            int found = -1;
            for (auto& [entryId, roomEntry] : rooms_)
            {
                if (player(stringField(fields, 0), entryId))
                {
                    found = entryId;
                }
            }
            send(client, 308, {int32_t(found)});
        }
        else if (messageId == 109)
        {
            try
            {
                storage_.save_profile(client.user, fields);
            }
            catch (const std::exception&)
            {
                send(client, 313, {int32_t(-1)});
                return;
            }
            send(client, 313, {int32_t(1)}); // The recovered edit-result success value is 1, unlike login's 0.
            auto profile = storage_.profile(client.user);
            for (auto& [entryId, roomPlayer] : clients_)
            {
                if (roomPlayer.owner == client.id && roomPlayer.service == Service::Room && !roomPlayer.user.empty())
                {
                    roomPlayer.record[2] = profile[2];
                    roomPlayer.record[3] = profile[3];
                    roomPlayer.record[4] = profile[4];
                    refresh(roomPlayer);
                }
            }
        }
        else if (messageId == 622)
        {
            try
            {
                storage_.save_membership(client.user, fields, 622);
            }
            catch (const std::exception&)
            {
                send(client, 315, {int32_t(-1), client.user});
                return;
            }
            send(client, 314, storage_.membership(client.user));
            send(client, 315, {int32_t(1), client.user});
        }
        else if (messageId == 632)
        {
            if (!valid_name(stringField(fields, 0)) || !accounts_.contains(stringField(fields, 0)))
            {
                return;
            }
            auto member = storage_.membership(stringField(fields, 0));
            // SDK tPublicInfo uses fixed buffers; reserve a NUL and expose only its public membership projection.
            auto bounded = [&](size_t field, size_t maximum)
            {
                return stringField(member, field).substr(0, maximum);
            };
            Fields info{bounded(0, 24), bounded(5, 49), bounded(6, 3), bounded(7, 29), member[11], member[12]};
            info.insert(info.end(), member.begin() + 14, member.end());
            send(client, 316, info);
        }
        else if (messageId == 631)
        {
            if (Clock::now() - client.last_complaint < std::chrono::seconds(30))
            {
                return;
            }
            storage_.complaint(client.user, fields);
            client.last_complaint = Clock::now(); // Sender never waits for a wire acknowledgement.
        }
        else if (messageId == 801 || messageId == 802 || messageId == 804)
        {
            moderation(client, messageId, fields);
        }
        else if (messageId == 104 && key(stringField(fields, 0)) == key(client.user))
        {
            client.closing = true;
        }
    }

    void Server::enter(Client& member, Game& targetGame)
    {
        if (game(member.id, member.room) || targetGame.started ||
            targetGame.members.size() >= static_cast<size_t>(integerField(targetGame.listing, 1)))
        {
            throw std::runtime_error("Invalid game admission");
        }
        int slot = 0;
        for (;;)
        {
            bool used = false;
            for (auto& [memberId, assignedSlot] : targetGame.slots)
            {
                if (assignedSlot == slot)
                {
                    used = true;
                }
            }
            if (!used)
            {
                break;
            }
            ++slot;
        }
        targetGame.members.push_back(member.id);
        targetGame.slots[member.id] = slot;
        broadcast(member.room, 411, {targetGame.listing[0], int32_t(0), int32_t(slot), member.user});
        // 411 rebuilds the client's player object, so its endpoint must be restored with 111.
        refresh(member);
    }

    void Server::leave(Client& member)
    {
        auto& games = rooms_.at(member.room).games;
        auto currentGame = game(member.id, member.room);
        if (!currentGame)
        {
            return;
        }
        auto gameName = stringField(currentGame->listing, 0);
        broadcast(member.room, 412, {gameName, int32_t(0), int32_t(currentGame->slots.at(member.id)), member.user});
        currentGame->slots.erase(member.id);
        currentGame->members.erase(std::remove(currentGame->members.begin(), currentGame->members.end(), member.id),
                                   currentGame->members.end());
        // 412 also rebuilds the room entry; preserve its endpoint for a later game join.
        refresh(member);
        for (auto& [connectionId, connection] : clients_)
        {
            if (connection.owner == member.owner && connection.room == member.room &&
                (connection.service == Service::Control || connection.service == Service::Data))
            {
                connection.ready = false;
                connection.closing = true;
            }
        }
        for (auto requestIterator = pending_.begin(); requestIterator != pending_.end();)
        {
            if (requestIterator->first == member.id || requestIterator->second.host == member.id)
            {
                auto requester = clients_.find(requestIterator->first);
                if (requester != clients_.end() && !requestIterator->second.invite)
                {
                    send(requester->second, 410, {int32_t(-1)});
                }
                requestIterator = pending_.erase(requestIterator);
            }
            else
            {
                ++requestIterator;
            }
        }
        if (currentGame->members.empty())
        {
            games.erase(key(gameName));
            broadcast(member.room, 406, {gameName});
        }
    }

    void Server::room(Client& client, unsigned messageId, const Fields& fields)
    {
        auto& currentRoom = rooms_.at(client.room);
        if (messageId == 111 && client.user.empty())
        {
            auto owner = identify(client, stringField(fields, 1));
            int count = 0;
            for (auto& [entryId, otherPlayer] : clients_)
            {
                if (!otherPlayer.closing && otherPlayer.service == Service::Room && otherPlayer.room == client.room &&
                    !otherPlayer.user.empty())
                {
                    ++count;
                }
            }
            if (!owner || owner->admitted != client.room || Clock::now() > owner->probe_expires ||
                count >= currentRoom.capacity)
            {
                throw std::runtime_error("Room admission required");
            }
            for (auto& [entryId, otherPlayer] : clients_)
            {
                if (!otherPlayer.closing && otherPlayer.service == Service::Room && otherPlayer.owner == owner->id &&
                    !otherPlayer.user.empty())
                {
                    throw std::runtime_error("Already in a room");
                }
            }
            if (integerField(fields, 6) < 0 || integerField(fields, 6) > 65535)
            {
                throw std::runtime_error("Invalid gameplay port");
            }
            client.user = owner->user;
            client.owner = owner->id;
            owner->admitted = 0;
            client.record = fields;
            // Neither identity string nor profile privilege bits may replace the authenticated account owner.
            client.record[0] = client.user;
            client.record[1] = client.user;
            auto profile = storage_.profile(client.user);
            client.record[2] = profile[2];
            client.record[3] = profile[3];
            client.record[4] = profile[4];
            owner->room_risks.insert(client.room);
            currentRoom.used = true;
            Fields configuration(14, int32_t(0));
            configuration[0] = int32_t(client.room);
            configuration[1] = int32_t(2);
            configuration[2] = int32_t(4);
            configuration[13] = int32_t(3);
            // 401 positions 1/2 bound player choices; position 13 == 3 enables the client's UDP reachability gate.
            send(client, 401, configuration);
            send(client, 111, client.record);
            send(client, 201, {client.user});
            send(client, 402, {int32_t(client.room), int32_t(0)});
            for (auto& [entryId, other] : clients_)
            {
                if (entryId != client.id && !other.closing && other.service == Service::Room &&
                    other.room == client.room && !other.user.empty())
                {
                    // Remote 201 creates the object that 111 updates; reversing them loses the endpoint.
                    send(client, 201, {other.user});
                    send(client, 111, record_for(other, client));
                    send(other, 201, {client.user});
                    send(other, 111, record_for(client, other));
                }
            }
            for (auto& [name, targetGame] : currentRoom.games)
            {
                send(client, 403, targetGame.listing);
                for (auto member : targetGame.members)
                {
                    auto& other = clients_.at(member);
                    send(client, 411,
                         {targetGame.listing[0], int32_t(0), int32_t(targetGame.slots.at(member)), other.user});
                    send(client, 111, record_for(other, client));
                }
            }
            send(client, 207);
            population(client.room);
            return;
        }
        if (client.user.empty())
        {
            throw std::runtime_error("Player record required");
        }
        if (messageId == 107)
        {
            // This ping response proves the client processed its new room before receiving any old-room 309.
            auto owner = clients_.find(client.owner);
            if (owner != clients_.end())
            {
                owner->second.room_risks = {client.room};
                for (auto removalIterator = owner->second.removals.begin();
                     removalIterator != owner->second.removals.end();)
                {
                    if (removalIterator->first == client.room)
                    {
                        ++removalIterator;
                        continue;
                    }
                    send(owner->second, 309, {removalIterator->second});
                    removalIterator = owner->second.removals.erase(removalIterator);
                }
            }
            return;
        }
        if (messageId == 701)
        {
            broadcast(client.room, 701, {client.user, fields[1]});
        }
        else if (messageId == 702)
        {
            if (auto target = player(stringField(fields, 0), client.room))
            {
                send(*target, 702, {target->user, client.user, fields[2]});
            }
        }
        else if (messageId == 634 && key(stringField(fields, 0)) == key(client.user))
        {
            client.probe_expires = Clock::now() + std::chrono::seconds(30);
            client.probe_replies = 0;
            send(client, 110, {config_.advertise, int32_t(config_.port + 5)});
        }
        else if (messageId == 605)
        {
            if (!valid_display_name(stringField(fields, 0)) || game(client.id, client.room) ||
                currentRoom.games.count(key(stringField(fields, 0))) || integerField(fields, 1) < 2 ||
                integerField(fields, 1) > 4 || currentRoom.games.size() >= 32)
            {
                send(client, 404, {int32_t(-1), std::string(), int32_t(0)});
                return;
            }
            Game created;
            created.listing = {fields[0],
                               fields[1],
                               fields[2],
                               fields[3],
                               fields[4],
                               config_.advertise,
                               int32_t(currentRoom.port + 1)};
            auto& targetGame = currentRoom.games.emplace(key(stringField(fields, 0)), std::move(created)).first->second;
            broadcast(client.room, 403, targetGame.listing);
            send(client, 404, {int32_t(0), config_.advertise, int32_t(currentRoom.port + 1)});
            enter(client, targetGame);
        }
        else if (messageId == 608 || messageId == 606)
        {
            auto found = currentRoom.games.find(key(stringField(fields, 0)));
            Client* target = messageId == 608 ? &client : player(stringField(fields, 3), client.room);
            bool valid = found != currentRoom.games.end() && target && !found->second.started &&
                         found->second.members.size() < static_cast<size_t>(integerField(found->second.listing, 1)) &&
                         !game(target->id, client.room) && !pending_.count(target->id);
            if (messageId == 608 && found != currentRoom.games.end() && found->second.join_permission == 1)
            {
                valid = false;
            }
            if (messageId == 606 && (found == currentRoom.games.end() || !found->second.slots.count(client.id)))
            {
                valid = false;
            }
            if (!valid)
            {
                if (messageId == 608)
                {
                    send(client, 410, {int32_t(-1)});
                }
                else
                {
                    send(client, 408, {int32_t(0), fields[3]});
                }
                return;
            }
            auto& targetGame = found->second;
            if (messageId == 608 && targetGame.join_permission == 2)
            {
                send(client, 410, {int32_t(0)});
                enter(client, targetGame);
                return;
            }
            auto host_id = messageId == 608 ? targetGame.members.front() : client.id;
            pending_[target->id] = {host_id, client.room, key(stringField(fields, 0)),
                                    Clock::now() + std::chrono::seconds(120), messageId == 606};
            if (messageId == 608)
            {
                send(clients_.at(host_id), 409, {targetGame.listing[0], fields[2], fields[3], client.user});
            }
            else
            {
                send(*target, 407, {targetGame.listing[0], fields[1], fields[2], client.user});
            }
        }
        else if (messageId == 609 || messageId == 607)
        {
            auto target = messageId == 609 ? player(stringField(fields, 0), client.room) : &client;
            if (!target)
            {
                return;
            }
            auto pending = pending_.find(target->id);
            if (pending == pending_.end())
            {
                return;
            }
            auto request = pending->second;
            if (request.invite != (messageId == 607) || request.room != client.room)
            {
                return;
            }
            auto host = clients_.find(request.host);
            if (host == clients_.end() ||
                (messageId == 609 ? request.host != client.id : key(host->second.user) != key(stringField(fields, 0))))
            {
                return;
            }
            pending_.erase(pending);
            auto found = currentRoom.games.find(request.game);
            // A prompt is not a reservation: capacity, host ownership and game state can change before the reply.
            bool accept = integerField(fields, 1) == 1 && Clock::now() <= request.expires &&
                          found != currentRoom.games.end() && !found->second.started &&
                          found->second.slots.count(request.host) && !game(target->id, client.room) &&
                          found->second.members.size() < static_cast<size_t>(integerField(found->second.listing, 1));
            if (messageId == 609 && found != currentRoom.games.end() && found->second.join_permission == 1)
            {
                accept = false;
            }
            if (messageId == 609)
            {
                send(*target, 410, {int32_t(accept ? 0 : -1)});
            }
            else
            {
                send(host->second, 408, {int32_t(accept ? 1 : 0), target->user});
            }
            if (accept)
            {
                enter(*target, found->second);
            }
            else if (auto control = channel(*target, Service::Control))
            {
                control->closing = true;
            }
        }
        else if (messageId == 615)
        {
            leave(client);
        }
        else if (messageId == 106 && key(stringField(fields, 0)) == key(client.user))
        {
            leave(client);
        }
    }

    void Server::transport(Client& client, unsigned messageId, const Fields& fields)
    {
        auto& currentRoom = rooms_.at(client.room);
        if (client.user.empty() && ((client.service == Service::Control && messageId == 102) ||
                                    (client.service == Service::Data && messageId == 601)))
        {
            auto owner = identify(client, stringField(fields, 0));
            auto roomPlayer = player(stringField(fields, 0), client.room);
            if (!owner || !roomPlayer || roomPlayer->owner != owner->id)
            {
                throw std::runtime_error("Game identity required");
            }
            auto pending = pending_.find(roomPlayer->id);
            bool invited =
                pending != pending_.end() && pending->second.invite && Clock::now() < pending->second.expires;
            // The original invite flow opens transport before 607; pending invites permit setup, not membership.
            if (!game(roomPlayer->id, client.room) && !invited)
            {
                throw std::runtime_error("Game membership required");
            }
            client.user = roomPlayer->user;
            client.owner = owner->id;
            for (auto& [connectionId, oldConnection] : clients_)
            {
                if (connectionId != client.id && oldConnection.service == client.service &&
                    oldConnection.room == client.room && oldConnection.owner == client.owner)
                {
                    oldConnection.closing = true;
                }
            }
            if (client.service == Service::Control)
            {
                send(client, 103,
                     {std::string(), config_.advertise, int32_t(currentRoom.port + 2), int32_t(currentRoom.port + 2)});
            }
            else
            {
                send(client, 207); // Data-channel 601 identifies transport; a lobby-style 311 here breaks setup.
            }
            return;
        }
        if (client.user.empty())
        {
            throw std::runtime_error("Transport identification required");
        }
        auto roomPlayer = player(client.user, client.room);
        if (!roomPlayer || roomPlayer->owner != client.owner)
        {
            throw std::runtime_error("Player disconnected");
        }
        if (messageId == 107)
        {
            if (client.service == Service::Data)
            {
                bool initializing = !client.ready;
                client.ready = true;
                // Later data heartbeats must not replay the control-side initialization notification.
                if (auto control = channel(client, Service::Control); control && initializing)
                {
                    send(*control, 201, {client.user});
                    send(*control, 207);
                }
            }
            else if (auto data = channel(client, Service::Data); data && data->ready)
            {
                client.ready = true;
            }
        }
        else if (messageId == 627 && client.service == Service::Control)
        {
            auto currentGame = game(roomPlayer->id, client.room);
            if (currentGame && currentGame->members.front() == roomPlayer->id &&
                key(stringField(currentGame->listing, 0)) == key(stringField(fields, 0)))
            {
                // SDK parameter 1: deny all (1), open (2), owner approval (5).
                currentGame->reported_state = integerField(fields, 1);
                currentGame->reported_state_aux = integerField(fields, 2);
                if (integerField(fields, 1) == 1 &&
                    (integerField(fields, 2) == 1 || integerField(fields, 2) == 2 || integerField(fields, 2) == 5))
                {
                    currentGame->join_permission = integerField(fields, 2);
                }
            }
        }
        else if (messageId == 619 && client.service == Service::Control)
        {
            auto currentGame = game(roomPlayer->id, client.room);
            if (!currentGame || currentGame->members.front() != roomPlayer->id || currentGame->started ||
                currentGame->members.size() < 2)
            {
                return;
            }
            for (auto memberId : currentGame->members)
            {
                auto control = channel(clients_.at(memberId), Service::Control);
                auto data = channel(clients_.at(memberId), Service::Data);
                if (!control || !data || !control->ready || !data->ready)
                {
                    return;
                }
            }
            currentGame->started = true;
            // 405 launches through room callbacks even though 619 arrived on game control.
            for (auto memberId : currentGame->members)
            {
                send(clients_.at(memberId), 405, {currentGame->listing[0]});
            }
        }
        else if ((messageId == 703 || messageId == 704) && client.service == Service::Data && client.ready)
        {
            auto currentGame = game(roomPlayer->id, client.room);
            if (!currentGame)
            {
                return;
            }
            if (messageId == 704 && integerField(fields, 1) != 0)
            {
                return;
            }
            // Experimental routing: send the actual sender slot plus a zero auxiliary field; other meanings are
            // unrecovered.
            auto blob = fields.at(messageId == 703 ? 0 : 2);
            for (auto memberId : currentGame->members)
            {
                if (memberId != roomPlayer->id &&
                    (messageId == 703 || currentGame->slots.at(memberId) == integerField(fields, 0)))
                {
                    auto data = channel(clients_.at(memberId), Service::Data);
                    if (data && data->ready)
                    {
                        send(*data, 704, {int32_t(currentGame->slots.at(roomPlayer->id)), int32_t(0), blob});
                    }
                }
            }
        }
        else if ((messageId == 106 || messageId == 104) && key(stringField(fields, 0)) == key(client.user))
        {
            leave(*roomPlayer);
            client.closing = true;
        }
    }
} // namespace sigs::server
