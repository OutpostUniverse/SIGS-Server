#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using NativeSocket = SOCKET;
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
using NativeSocket = int;
#endif
#include "SIGSCore.h"

namespace sigs::core
{
    NativeSocket native(std::intptr_t);
    void set_nonblocking(NativeSocket);
    void wait_socket(NativeSocket, bool, std::chrono::steady_clock::time_point);
    bool socket_would_block();
    sockaddr_in ipv4_address(const std::string&, std::uint16_t);
} // namespace sigs::core
