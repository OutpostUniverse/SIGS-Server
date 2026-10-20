#pragma once
#include <map>
#include <string>
#include <vector>

namespace sigs::server
{
    std::string key(std::string value);
    bool valid_name(const std::string&);
    std::string random_secret();
    std::string sha256(const std::vector<unsigned char>&);

    class Accounts
    {
        // A startup snapshot: normalized account name -> hex salt / PBKDF2-SHA256 digest, never plaintext.
        std::map<std::string, std::pair<std::string, std::string>> entries_;

    public:
        explicit Accounts(const std::string& path);
        bool verify(const std::string& name, const std::string& password) const;
        // File replacement is atomic but not coordinated with other writers; run administrative edits offline.
        static void add(const std::string& path, const std::string& name, const std::string& password);
        static void reset(const std::string& path, const std::string& name, const std::string& password);

        bool contains(const std::string& name) const
        {
            return entries_.count(key(name)) != 0;
        }
    };
} // namespace sigs::server
