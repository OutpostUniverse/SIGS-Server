#include "Updates.h"
#include "Storage.h"
#include "Accounts.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <set>

namespace sigs::server
{
    std::vector<Update> load_updates(const std::string& file)
    {
        std::vector<Update> updates;
        if (file.empty())
        {
            return updates;
        }
        std::ifstream input(file);
        if (!input)
        {
            throw std::runtime_error("Cannot open updates manifest");
        }
        std::string line;
        size_t total = 0;
        std::set<std::string> routes;
        while (std::getline(input, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }
            Update update{};
            std::string package, expected, trailing;
            int required;
            std::istringstream record(line);
            if (!(record >> update.kind >> std::quoted(update.identifier) >> std::quoted(update.from) >>
                  std::quoted(update.target) >> required >> std::quoted(package) >> expected) ||
                record >> trailing || (update.kind != "game" && update.kind != "sigs") ||
                (required != 0 && required != 1) || update.identifier.empty() || update.from.empty() ||
                update.target.empty() || update.target.size() > 64 || update.target == update.from ||
                updates.size() >= 16)
            {
                throw std::runtime_error("Invalid update route");
            }
            auto route = update.kind + '\n' + key(update.identifier) + '\n' + update.from;
            // Ambiguous exact routes must fail startup rather than silently choose a different executable package.
            if (!routes.insert(route).second)
            {
                throw std::runtime_error("Duplicate update route");
            }
            update.required = required != 0;
            // Only operator-configured paths are read; this hash validates local configuration, not a client signature.
            update.bytes = read_file(std::filesystem::path(file).parent_path() / package, 64 * 1024 * 1024);
            if (update.bytes.empty() || sha256(update.bytes) != key(expected))
            {
                throw std::runtime_error("Update package checksum mismatch or empty package");
            }
            total += update.bytes.size();
            // Packages remain resident for stable transfers, so bound their combined memory as well as each file.
            if (total > 128 * 1024 * 1024)
            {
                throw std::runtime_error("Update package total exceeds limit");
            }
            updates.push_back(std::move(update));
        }
        if (!input.eof())
        {
            throw std::runtime_error("Cannot read updates manifest");
        }
        return updates;
    }
} // namespace sigs::server
