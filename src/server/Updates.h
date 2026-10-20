#pragma once
#include "SIGSCore.h"

namespace sigs::server
{
    struct Update
    {
        std::string kind, identifier, from, target;
        bool required;
        sigs::core::Bytes
            bytes; // Verified startup snapshot; active transfers cannot observe later package-file changes.
    };

    std::vector<Update> load_updates(const std::string&);
} // namespace sigs::server
