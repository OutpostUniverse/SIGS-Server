#include "SIGSCore.h"

namespace sigs::core
{
    void CommunicationManager::add(unsigned id, TcpSocket socket)
    {
        if (polling_)
        {
            throw std::logic_error("Cannot change connections inside a dispatch callback");
        }
        if (connections_.count(id))
        {
            throw std::invalid_argument("Duplicate connection");
        }
        connections_.emplace(id, Connection(std::move(socket)));
    }

    void CommunicationManager::remove(unsigned id)
    {
        if (polling_)
        {
            throw std::logic_error("Cannot change connections inside a dispatch callback");
        }
        connections_.erase(id);
    }

    void CommunicationManager::send(unsigned id, const Packet& packet)
    {
        if (polling_)
        {
            throw std::logic_error("Queue sends until poll returns");
        }
        try
        {
            connections_.at(id).send(packet);
        }
        catch (...)
        {
            connections_.erase(id);
            throw;
        }
    }

    bool CommunicationManager::poll(unsigned id, int timeout)
    {
        if (polling_)
        {
            throw std::logic_error("Recursive polling is unsupported");
        }
        polling_ = true;
        try
        {
            bool alive = connections_.at(id).poll(
                [this](const Packet& packet)
                {
                    dispatcher_.dispatch(packet);
                },
                timeout);
            polling_ = false;
            if (!alive)
            {
                connections_.erase(id);
            }
            return alive;
        }
        catch (...)
        {
            polling_ = false;
            connections_.erase(id);
            throw;
        }
    }
} // namespace sigs::core
