#include "SIGSCore.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace sigs::core
{
    std::string utc_timestamp()
    {
        auto time = std::time(nullptr);
        std::tm t{};
#ifdef _WIN32
        if (gmtime_s(&t, &time))
        {
            throw std::runtime_error("UTC conversion failed");
        }
#else
        if (!gmtime_r(&time, &t))
        {
            throw std::runtime_error("UTC conversion failed");
        }
#endif
        std::ostringstream s;
        s << std::put_time(&t, "%Y-%m-%dT%H:%M:%SZ");
        return s.str();
    }
} // namespace sigs::core
