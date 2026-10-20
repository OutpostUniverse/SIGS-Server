#include "SIGSCore.h"

namespace sigs::core
{
    void append_be32(Bytes& bytes, std::uint32_t value)
    {
        for (int shift = 24; shift >= 0; shift -= 8)
        {
            bytes.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    }

    std::uint32_t read_be32(const Bytes& bytes, std::size_t offset)
    {
        if (offset > bytes.size() || bytes.size() - offset < 4)
        {
            throw std::runtime_error("Truncated integer");
        }
        return (std::uint32_t(bytes[offset]) << 24) | (std::uint32_t(bytes[offset + 1]) << 16) |
               (std::uint32_t(bytes[offset + 2]) << 8) | bytes[offset + 3];
    }
} // namespace sigs::core
