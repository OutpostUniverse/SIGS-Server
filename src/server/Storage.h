#pragma once
#include "SIGSCore.h"
#include <filesystem>

namespace sigs::server
{
    using Fields = std::vector<sigs::core::Value>;
    // Replace through a sibling temporary file so a failed write does not truncate the previous record.
    void atomic_write(const std::filesystem::path&, const sigs::core::Bytes&);
    sigs::core::Bytes read_file(const std::filesystem::path&, size_t maximum);

    // Records reuse recovered field layouts; account identity always comes from the authenticated caller.
    class Storage
    {
        std::filesystem::path root_;
        std::filesystem::path user_path(const std::string&, const char*) const;

    public:
        explicit Storage(const std::string& directory);
        Fields membership(const std::string&) const;
        void save_membership(const std::string&, const Fields&, unsigned message);
        Fields profile(const std::string&) const;
        void save_profile(const std::string&, const Fields&);
        void complaint(const std::string&, const Fields&);
    };
} // namespace sigs::server
