#include "SIGSCore.h"
#include <cstring>
#include <limits>

namespace sigs::core
{
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559, "binary32 required");

    Bytes encode_fields(const std::string& formatString, const std::vector<Value>& values)
    {
        auto formatTokens = parse_format(formatString);
        if (formatTokens.size() != values.size())
        {
            throw std::invalid_argument("Field count");
        }
        Bytes payload;
        for (std::size_t fieldIndex = 0; fieldIndex < formatTokens.size(); ++fieldIndex)
        {
            auto& token = formatTokens[fieldIndex];
            auto& value = values[fieldIndex];
            if (token == "%s")
            {
                auto& text = std::get<std::string>(value);
                if (text.size() > MaxFrame - payload.size() - 1)
                {
                    throw std::length_error("Payload limit");
                }
                append_string(payload, text);
            }
            else if (token == "%i")
            {
                append_be32(payload, static_cast<std::uint32_t>(std::get<std::int32_t>(value)));
            }
            else if (token == "%l")
            {
                auto scalar = std::get<std::uint16_t>(value);
                payload.push_back(static_cast<std::uint8_t>(scalar >> 8));
                payload.push_back(static_cast<std::uint8_t>(scalar));
            }
            else if (token == "%c")
            {
                payload.push_back(std::get<std::uint8_t>(value));
            }
            else if (token == "%f")
            {
                // Transfer IEEE-754 bits, not a numeric float-to-integer conversion or an aliasing pointer cast.
                auto scalar = std::get<float>(value);
                std::uint32_t bits;
                std::memcpy(&bits, &scalar, 4);
                append_be32(payload, bits);
            }
            else
            {
                auto& data = std::get<Bytes>(value);
                if (data.size() > MaxFrame - 4 || payload.size() > MaxFrame - 4 - data.size())
                {
                    throw std::length_error("Binary limit");
                }
                append_be32(payload, static_cast<std::uint32_t>(data.size()));
                payload.insert(payload.end(), data.begin(), data.end());
            }
            if (payload.size() > MaxFrame - 12)
            {
                throw std::length_error("Payload limit");
            }
        }
        return payload;
    }

    std::vector<Value> decode_fields(const std::string& formatString, const Bytes& payload)
    {
        if (payload.size() > MaxFrame - 12)
        {
            throw std::length_error("Payload limit");
        }
        auto formatTokens = parse_format(formatString);
        std::vector<Value> fields;
        std::size_t offset = 0;
        for (auto& token : formatTokens)
        {
            if (token == "%s")
            {
                fields.emplace_back(read_string(payload, offset));
                continue;
            }
            auto scalar = token == "%l" ? 2u : token == "%c" ? 1u : 4u;
            if (offset > payload.size() || payload.size() - offset < scalar)
            {
                throw std::runtime_error("Truncated field");
            }
            if (token == "%l")
            {
                fields.emplace_back(static_cast<std::uint16_t>((payload[offset] << 8) | payload[offset + 1]));
                offset += 2;
            }
            else if (token == "%c")
            {
                fields.emplace_back(payload[offset++]);
            }
            else
            {
                auto bits = read_be32(payload, offset);
                offset += 4;
                if (token == "%i")
                {
                    std::int32_t value;
                    std::memcpy(&value, &bits, 4);
                    fields.emplace_back(value);
                }
                else if (token == "%f")
                {
                    float value;
                    std::memcpy(&value, &bits, 4);
                    fields.emplace_back(value);
                }
                else
                {
                    if (bits > payload.size() - offset)
                    {
                        throw std::runtime_error("Truncated block");
                    }
                    fields.emplace_back(Bytes(payload.begin() + offset, payload.begin() + offset + bits));
                    offset += bits;
                }
            }
        }
        if (offset != payload.size())
        {
            // Surplus bytes indicate a wrong profile/layout; silently accepting them would conceal protocol drift.
            throw std::runtime_error("Trailing payload");
        }
        return fields;
    }
} // namespace sigs::core
