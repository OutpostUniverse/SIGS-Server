#include "SIGSCore.h"
#include <sstream>

namespace sigs::core
{
    std::vector<std::string> parse_format(const std::string& formatString)
    {
        std::istringstream input(formatString);
        std::vector<std::string> formatTokens;
        std::string token;
        while (input >> token)
        {
            if (token != "%s" && token != "%i" && token != "%l" && token != "%c" && token != "%f" && token != "%b")
            {
                throw std::invalid_argument("Unsupported wire token: " + token);
            }
            formatTokens.push_back(token);
        }
        return formatTokens;
    }
} // namespace sigs::core
