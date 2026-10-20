#include "SIGSCore.h"

namespace sigs::core
{
    void Dispatcher::on(unsigned id, const std::function<void(const Packet&, const std::vector<Value>&)>& handler)
    {
        if (!handler)
        {
            throw std::invalid_argument("Empty handler");
        }
        handlers_[id] = handler;
    }

    bool Dispatcher::dispatch(const Packet& packet) const
    {
        auto handler = handlers_.find(packet.id);
        if (handler == handlers_.end())
        {
            return false;
        }
        auto definition = FindMessageDefinition(registry_, packet.id);
        if (!definition)
        {
            throw std::invalid_argument("Unknown registered layout");
        }
        handler->second(packet, decode_fields(definition->format, packet.payload));
        return true;
    }
} // namespace sigs::core
