#include "SIGSCore.h"
#include <iostream>
#include <limits>
using namespace sigs::core;
static int checks = 0;

void check(bool ok)
{
    ++checks;
    if (!ok)
    {
        throw std::runtime_error("Check failed " + std::to_string(checks));
    }
}

template <class F> void rejects(F operation)
{
    bool thrown = false;
    try
    {
        operation();
    }
    catch (const std::exception&)
    {
        thrown = true;
    }
    check(thrown);
}

int main()
{
    try
    {
        size_t count = 0;
        GetMessageDefinitions(SierraNW, &count);
        check(count == 118);
        auto definitions = GetMessageDefinitions(SierraNW, &count);
        for (size_t definitionIndex = 0; definitionIndex < count; ++definitionIndex)
        {
            auto other = FindMessageDefinition(SNWValid, definitions[definitionIndex].id);
            check(other && std::string(other->format) == definitions[definitionIndex].format);
        }
        check(!FindMessageDefinition(SierraNW, 999999));
        Bytes ping{0, 0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 207, 1, 0, 0, 0};
        check(encode_packet({207, {}}) == ping);
        check(decode_packet(ping).id == 207);
        Bytes scalar{0x41, 0, 0xff, 0xff, 0xff, 0xff, 0x12, 0x34, 0xab, 0x3f, 0x80, 0, 0};
        check(encode_fields("%s %i %l %c %f", {std::string("A"), std::int32_t(-1), std::uint16_t(0x1234),
                                               std::uint8_t(0xab), 1.0f}) == scalar);
        auto decodedFields = decode_fields("%s %i %l %c %f", scalar);
        check(std::get<std::int32_t>(decodedFields[1]) == -1);
        check(std::get<float>(decodedFields[4]) == 1.0f);
        Bytes blob{0, 0, 0, 3, 0, 0xff, 0x42};
        check(encode_fields("%b", {Bytes{0, 0xff, 0x42}}) == blob);
        check(std::get<Bytes>(decode_fields("%b", blob)[0]) == Bytes({0, 0xff, 0x42}));
        rejects(
            []
            {
                encode_fields("%t", {});
            });
        rejects(
            []
            {
                encode_fields("%lf", {});
            });
        rejects(
            []
            {
                encode_fields("%i", {});
            });
        rejects(
            []
            {
                encode_fields("%s", {std::string("a\0b", 3)});
            });
        rejects(
            []
            {
                decode_fields("%s", Bytes{65});
            });
        rejects(
            []
            {
                decode_fields("%i", Bytes{0});
            });
        rejects(
            []
            {
                decode_fields("", Bytes{0});
            });
        rejects(
            []
            {
                decode_fields("%b", Bytes{0, 0, 0, 9});
            });
        auto bad = ping;
        bad[7] = 13;
        rejects(
            [&]
            {
                decode_packet(bad);
            });
        bad = ping;
        bad.push_back(0);
        rejects(
            [&]
            {
                decode_packet(bad);
            });
        FrameStream stream;
        int received = 0;
        auto callback = [&](const Packet& p)
        {
            check(p.id == 207);
            ++received;
        };
        for (auto byte : ping)
        {
            stream.feed(Bytes{byte}, callback);
        }
        check(received == 1);
        stream.finish();
        Bytes both = ping;
        both.insert(both.end(), ping.begin(), ping.end());
        stream.feed(both, callback);
        check(received == 3);
        FrameStream partial;
        partial.feed(Bytes{0, 0}, callback);
        rejects(
            [&]
            {
                partial.finish();
            });
        FrameStream oversized;
        rejects(
            [&]
            {
                oversized.feed(Bytes{0x7f, 0xff, 0xff, 0xff}, callback);
            });
        Dispatcher dispatch;
        int handled = 0;
        dispatch.on(609,
                    [&](const Packet&, const std::vector<Value>& receivedFields)
                    {
                        check(std::get<std::string>(receivedFields[0]) == "Alice");
                        check(std::get<std::int32_t>(receivedFields[1]) == 1);
                        ++handled;
                    });
        check(dispatch.dispatch({609, encode_fields("%s %i", {std::string("Alice"), std::int32_t(1)})}));
        check(handled == 1);
        check(!dispatch.dispatch({9999, {0xff}}));
        check(utc_timestamp().size() == 20);
        SocketRuntime runtime;
        auto listener = TcpSocket::listen_on("127.0.0.1", 0);
        auto client = TcpSocket::connect_to("127.0.0.1", listener.local_port());
        auto accepted = listener.accept_one();
        CommunicationManager manager;
        manager.add(1, std::move(accepted));
        int pings = 0;
        manager.dispatcher().on(207,
                                [&](const Packet&, const std::vector<Value>&)
                                {
                                    rejects(
                                        [&]
                                        {
                                            manager.remove(1);
                                        });
                                    ++pings;
                                });
        client.send_all(Bytes(ping.begin(), ping.begin() + 3));
        check(manager.poll(1));
        check(pings == 0);
        client.send_all(Bytes(ping.begin() + 3, ping.end()));
        check(manager.poll(1));
        check(pings == 1);
        manager.send(1, {107, {}});
        FrameStream replies;
        int reply_count = 0;
        while (!reply_count)
        {
            auto receivedBytes = client.receive();
            check(!receivedBytes.empty());
            replies.feed(receivedBytes,
                         [&](const Packet& p)
                         {
                             check(p.id == 107);
                             ++reply_count;
                         });
        }
        client.close();
        check(!manager.poll(1));
        check(manager.size() == 0);
        auto truncated_client = TcpSocket::connect_to("127.0.0.1", listener.local_port());
        manager.add(2, listener.accept_one());
        truncated_client.send_all(Bytes{0, 0});
        truncated_client.close();
        bool eof_error = false;
        try
        {
            while (manager.poll(2))
            {
            };
        }
        catch (const std::exception&)
        {
            eof_error = true;
        }
        check(eof_error);
        check(manager.size() == 0);
        auto idle = TcpSocket::connect_to("127.0.0.1", listener.local_port());
        auto idle_peer = listener.accept_one();
        rejects(
            [&]
            {
                idle_peer.receive(16, 20);
            });
        std::cout << checks << " checks passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
