// Replacement API, not Sierra's original ABI. See docs/core-reconstruction.md.
#pragma once
#include "MsgDef.h"
#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <map>
#include <chrono>
#include <stdexcept>

namespace sigs::core
{
    using Bytes = std::vector<std::uint8_t>;
    // String values preserve legacy bytes; callers choose their character encoding before serialization.
    using Value = std::variant<std::string, std::int32_t, std::uint16_t, std::uint8_t, float, Bytes>;
    constexpr std::size_t MaxFrame = 1024 * 1024; // Emulator resource limit, not a recovered Sierra constant.

    struct Packet
    {
        std::uint32_t id;
        Bytes payload;
        std::uint32_t flags = 0x01000000;
    };

    void append_be32(Bytes&, std::uint32_t);
    std::uint32_t read_be32(const Bytes&, std::size_t);
    void append_string(Bytes&, const std::string&);
    std::string read_string(const Bytes&, std::size_t&);
    std::vector<std::string> parse_format(const std::string&);
    Bytes encode_fields(const std::string&, const std::vector<Value>&);
    std::vector<Value> decode_fields(const std::string&, const Bytes&);
    Bytes encode_packet(const Packet&);
    Packet decode_packet(const Bytes&);

    class FrameStream
    {
        Bytes buffer_;
        std::size_t expected_ = 4;

    public:
        // Callback exceptions propagate; the emitted packet has already been consumed.
        void feed(const Bytes&, const std::function<void(const Packet&)>&);
        void finish() const;
    };

    std::string utc_timestamp(); // Modern logging utility, NOT recovered %t encoding.

    class SocketRuntime
    {
    public:
        SocketRuntime();
        ~SocketRuntime();
        SocketRuntime(const SocketRuntime&) = delete;
        SocketRuntime& operator=(const SocketRuntime&) = delete;
    };

    class TcpSocket
    {
        std::intptr_t handle_ = -1;

    public:
        explicit TcpSocket(std::intptr_t handle = -1) : handle_(handle)
        {
        }

        ~TcpSocket();
        TcpSocket(const TcpSocket&) = delete;
        TcpSocket& operator=(const TcpSocket&) = delete;
        TcpSocket(TcpSocket&&) noexcept;
        TcpSocket& operator=(TcpSocket&&) noexcept;
        static TcpSocket connect_to(const std::string& ipv4, std::uint16_t port, int timeout_ms = 5000);
        static TcpSocket listen_on(const std::string& ipv4, std::uint16_t port, int backlog = 16);
        TcpSocket accept_one(int timeout_ms = 5000) const;
        std::uint16_t local_port() const;

        // Borrowed handle for readiness polling; ownership and close remain with this TcpSocket.
        std::intptr_t native_handle() const
        {
            return handle_;
        }

        void send_all(const Bytes&, int timeout_ms = 5000) const;
        Bytes receive(std::size_t max_bytes = 65536, int timeout_ms = 5000) const;
        void close() noexcept;
    };

    class Connection
    {
        TcpSocket socket_;
        FrameStream frames_;
        bool failed_ = false;

    public:
        explicit Connection(TcpSocket socket) : socket_(std::move(socket))
        {
        }

        void send(const Packet&);
        // False means clean EOF. Any error permanently closes this connection.
        bool poll(const std::function<void(const Packet&)>&, int timeout_ms = 5000);
    };

    class Dispatcher
    {
        Registry registry_;
        std::map<unsigned, std::function<void(const Packet&, const std::vector<Value>&)>> handlers_;

    public:
        explicit Dispatcher(Registry r = SierraNW) : registry_(r)
        {
        }

        void on(unsigned, const std::function<void(const Packet&, const std::vector<Value>&)>&);
        // False leaves unhandled/unknown packets opaque; no speculative semantics.
        bool dispatch(const Packet&) const;
    };

    class CommunicationManager
    {
        Dispatcher dispatcher_;
        std::map<unsigned, Connection> connections_;
        bool polling_ = false;

    public:
        Dispatcher& dispatcher()
        {
            return dispatcher_;
        }

        void add(unsigned, TcpSocket);
        void remove(unsigned);
        void send(unsigned, const Packet&);
        bool poll(unsigned, int timeout_ms = 5000);

        std::size_t size() const
        {
            return connections_.size();
        }
    };
} // namespace sigs::core
