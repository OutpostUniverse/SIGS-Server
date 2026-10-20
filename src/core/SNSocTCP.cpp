#include "SocketInternal.h"
#include <algorithm>

namespace sigs::core
{
    TcpSocket::~TcpSocket()
    {
        close();
    }

    TcpSocket::TcpSocket(TcpSocket&& other) noexcept : handle_(other.handle_)
    {
        other.handle_ = -1;
    }

    TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept
    {
        if (this != &other)
        {
            close();
            handle_ = other.handle_;
            other.handle_ = -1;
        }
        return *this;
    }

    void TcpSocket::close() noexcept
    {
        if (handle_ != -1)
        {
#ifdef _WIN32
            closesocket(static_cast<NativeSocket>(handle_));
#else
            ::close(static_cast<NativeSocket>(handle_));
#endif
            handle_ = -1;
        }
    }

    static TcpSocket make_socket()
    {
        auto descriptor = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (descriptor == static_cast<NativeSocket>(-1))
        {
            throw std::runtime_error("socket failed");
        }
        TcpSocket result(static_cast<std::intptr_t>(descriptor));
        set_nonblocking(descriptor);
        return result;
    }

    TcpSocket TcpSocket::connect_to(const std::string& ipv4, std::uint16_t port, int timeoutMs)
    {
        auto endpoint = ipv4_address(ipv4, port);
        auto connection = make_socket();
        auto descriptor = native(connection.handle_);
        if (::connect(descriptor, reinterpret_cast<sockaddr*>(&endpoint), sizeof(endpoint)) != 0)
        {
            if (!socket_would_block())
            {
                throw std::runtime_error("connect failed");
            }
            wait_socket(descriptor, true, std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs));
            int error = 0;
#ifdef _WIN32
            int errorLength = sizeof(error);
#else
            socklen_t errorLength = sizeof(error);
#endif
            if (getsockopt(descriptor, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &errorLength) != 0 ||
                error)
            {
                throw std::runtime_error("connect failed");
            }
        }
        return connection;
    }

    TcpSocket TcpSocket::listen_on(const std::string& ipv4, std::uint16_t port, int backlog)
    {
        auto endpoint = ipv4_address(ipv4, port);
        auto connection = make_socket();
        auto descriptor = native(connection.handle_);
        if (bind(descriptor, reinterpret_cast<sockaddr*>(&endpoint), sizeof(endpoint)) || listen(descriptor, backlog))
        {
            throw std::runtime_error("bind/listen failed");
        }
        return connection;
    }

    TcpSocket TcpSocket::accept_one(int timeoutMs) const
    {
        auto descriptor = native(handle_);
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        for (;;)
        {
            wait_socket(descriptor, false, deadline);
            auto accepted = ::accept(descriptor, nullptr, nullptr);
            if (accepted != static_cast<NativeSocket>(-1))
            {
                TcpSocket connection(static_cast<std::intptr_t>(accepted));
                set_nonblocking(accepted);
                return connection;
            }
            if (!socket_would_block())
            {
                throw std::runtime_error("accept failed");
            }
        }
    }

    std::uint16_t TcpSocket::local_port() const
    {
        sockaddr_in endpoint{};
#ifdef _WIN32
        int addressLength = sizeof(endpoint);
#else
        socklen_t addressLength = sizeof(endpoint);
#endif
        if (getsockname(native(handle_), reinterpret_cast<sockaddr*>(&endpoint), &addressLength))
        {
            throw std::runtime_error("getsockname failed");
        }
        return ntohs(endpoint.sin_port);
    }

    void TcpSocket::send_all(const Bytes& bytes, int timeoutMs) const
    {
        auto descriptor = native(handle_);
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        std::size_t offset = 0;
        while (offset < bytes.size())
        {
            wait_socket(descriptor, true, deadline);
            int flags = 0;
#ifdef MSG_NOSIGNAL
            flags = MSG_NOSIGNAL;
#endif
            auto bytesTransferred =
                ::send(descriptor, reinterpret_cast<const char*>(bytes.data() + offset),
                       static_cast<int>(std::min<std::size_t>(bytes.size() - offset, 65536)), flags);
            if (bytesTransferred > 0)
            {
                offset += static_cast<std::size_t>(bytesTransferred);
            }
            else if (bytesTransferred == 0 || !socket_would_block())
            {
                throw std::runtime_error("send failed");
            }
        }
    }

    Bytes TcpSocket::receive(std::size_t maximum, int timeoutMs) const
    {
        if (!maximum || maximum > MaxFrame + 4)
        {
            throw std::invalid_argument("Receive size");
        }
        auto descriptor = native(handle_);
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        Bytes bytes(maximum);
        for (;;)
        {
            wait_socket(descriptor, false, deadline);
            auto bytesTransferred =
                recv(descriptor, reinterpret_cast<char*>(bytes.data()), static_cast<int>(bytes.size()), 0);
            if (bytesTransferred >= 0)
            {
                bytes.resize(static_cast<std::size_t>(bytesTransferred));
                return bytes;
            }
            if (!socket_would_block())
            {
                throw std::runtime_error("recv failed");
            }
        }
    }
} // namespace sigs::core
