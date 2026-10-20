#include "SIGSCore.h"

namespace sigs::core
{
    void Connection::send(const Packet& packet)
    {
        if (failed_)
        {
            throw std::runtime_error("Connection closed");
        }
        try
        {
            socket_.send_all(encode_packet(packet));
        }
        catch (...)
        {
            failed_ = true;
            socket_.close();
            throw;
        }
    }

    bool Connection::poll(const std::function<void(const Packet&)>& emit, int timeout)
    {
        if (failed_)
        {
            throw std::runtime_error("Connection closed");
        }
        try
        {
            auto bytes = socket_.receive(65536, timeout);
            if (bytes.empty())
            {
                frames_.finish();
                failed_ = true;
                socket_.close();
                return false;
            }
            frames_.feed(bytes, emit);
            return true;
        }
        catch (...)
        {
            failed_ = true;
            socket_.close();
            throw;
        }
    }
} // namespace sigs::core
