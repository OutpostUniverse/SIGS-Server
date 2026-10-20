#include "SIGSCore.h"
#include <algorithm>

namespace sigs::core
{
    void append_string(Bytes& bytes, const std::string& text)
    {
        if (text.find('\0') != std::string::npos)
        {
            throw std::invalid_argument("Embedded NUL");
        }
        bytes.insert(bytes.end(), text.begin(), text.end());
        bytes.push_back(0);
    }

    std::string read_string(const Bytes& bytes, std::size_t& offset)
    {
        if (offset > bytes.size())
        {
            throw std::runtime_error("String offset");
        }
        auto terminator = std::find(bytes.begin() + offset, bytes.end(), 0);
        if (terminator == bytes.end())
        {
            throw std::runtime_error("Unterminated string");
        }
        std::string text(bytes.begin() + offset, terminator);
        offset = std::size_t(terminator - bytes.begin()) + 1;
        return text;
    }
} // namespace sigs::core
