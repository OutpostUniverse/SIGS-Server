#include "SocketInternal.h"

namespace sigs::core
{
    SocketRuntime::SocketRuntime()
    {
#ifdef _WIN32
        WSADATA winsockData{};
        if (WSAStartup(MAKEWORD(2, 2), &winsockData))
        {
            throw std::runtime_error("WSAStartup failed");
        }
#endif
    }

    SocketRuntime::~SocketRuntime()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    sockaddr_in ipv4_address(const std::string& address, std::uint16_t port)
    {
        sockaddr_in endpoint{};
        endpoint.sin_family = AF_INET;
        endpoint.sin_port = htons(port);
        if (inet_pton(AF_INET, address.c_str(), &endpoint.sin_addr) != 1)
        {
            throw std::invalid_argument("Expected numeric IPv4");
        }
        return endpoint;
    }

    NativeSocket native(std::intptr_t handle)
    {
        if (handle == -1)
        {
            throw std::runtime_error("Closed socket");
        }
        return static_cast<NativeSocket>(handle);
    }

    bool socket_would_block()
    {
#ifdef _WIN32
        int errorCode = WSAGetLastError();
        return errorCode == WSAEWOULDBLOCK || errorCode == WSAEINPROGRESS;
#else
        return errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS || errno == EINTR;
#endif
    }

    void set_nonblocking(NativeSocket descriptor)
    {
#ifdef _WIN32
        u_long value = 1;
        if (ioctlsocket(descriptor, FIONBIO, &value))
        {
            throw std::runtime_error("Nonblocking failed");
        }
#else
        int flags = fcntl(descriptor, F_GETFL, 0);
        if (flags < 0 || fcntl(descriptor, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            throw std::runtime_error("Nonblocking failed");
        }
#endif
    }

    void wait_socket(NativeSocket descriptor, bool writing, std::chrono::steady_clock::time_point deadline)
    {
        for (;;)
        {
            auto remainingMicroseconds =
                std::chrono::duration_cast<std::chrono::microseconds>(deadline - std::chrono::steady_clock::now())
                    .count();
            if (remainingMicroseconds <= 0)
            {
                throw std::runtime_error("Socket timeout");
            }
            fd_set readSet, writeSet, errorSet;
            FD_ZERO(&readSet);
            FD_ZERO(&writeSet);
            FD_ZERO(&errorSet);
#ifndef _WIN32
            if (descriptor >= FD_SETSIZE)
            {
                throw std::runtime_error("Descriptor exceeds select limit");
            }
#endif
            FD_SET(descriptor, &errorSet);
            if (writing)
            {
                FD_SET(descriptor, &writeSet);
            }
            else
            {
                FD_SET(descriptor, &readSet);
            }
            timeval timeout{static_cast<long>(remainingMicroseconds / 1000000),
                            static_cast<long>(remainingMicroseconds % 1000000)};
#ifdef _WIN32
            int readyCount = select(0, &readSet, &writeSet, &errorSet, &timeout);
#else
            int readyCount = select(descriptor + 1, &readSet, &writeSet, &errorSet, &timeout);
#endif
            if (readyCount > 0)
            {
                return;
            }
            if (readyCount == 0)
            {
                throw std::runtime_error("Socket timeout");
            }
#ifndef _WIN32
            if (errno == EINTR)
            {
                continue;
            }
#endif
            throw std::runtime_error("select failed");
        }
    }
} // namespace sigs::core
