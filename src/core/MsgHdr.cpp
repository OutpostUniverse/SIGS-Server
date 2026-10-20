#include "SIGSCore.h"

namespace sigs::core
{
    Bytes encode_packet(const Packet& packet)
    {
        if (packet.payload.size() > MaxFrame - 12)
        {
            throw std::length_error("Frame limit");
        }
        Bytes frame;
        // Both length words count the 12-byte inner header and payload; the TCP outer word adds four more bytes.
        auto innerLength = static_cast<std::uint32_t>(12 + packet.payload.size());
        append_be32(frame, innerLength);
        append_be32(frame, innerLength);
        append_be32(frame, packet.id);
        append_be32(frame, packet.flags);
        frame.insert(frame.end(), packet.payload.begin(), packet.payload.end());
        return frame;
    }

    Packet decode_packet(const Bytes& frame)
    {
        if (frame.size() < 16)
        {
            throw std::runtime_error("Truncated header");
        }
        auto innerLength = read_be32(frame, 0);
        // Equal inner/outer lengths are the exercised SIGS profile; reject other framing modes rather than guess.
        if (innerLength < 12 || innerLength > MaxFrame || frame.size() != std::size_t(innerLength) + 4 ||
            read_be32(frame, 4) != innerLength)
        {
            throw std::runtime_error("Invalid framing");
        }
        return {read_be32(frame, 8), Bytes(frame.begin() + 16, frame.end()), read_be32(frame, 12)};
    }
} // namespace sigs::core
