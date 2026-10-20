#include "Storage.h"
#include "Accounts.h"
#include <windows.h>
#include <fstream>

namespace sigs::server
{
    using namespace sigs::core;

    Bytes read_file(const std::filesystem::path& path, size_t maximum)
    {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input)
        {
            throw std::runtime_error("Cannot open configured file");
        }
        auto length = input.tellg();
        if (length < 0 || static_cast<uint64_t>(length) > maximum)
        {
            throw std::runtime_error("Configured file exceeds limit");
        }
        Bytes bytes(static_cast<size_t>(length));
        input.seekg(0);
        if (!bytes.empty() && !input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
        {
            throw std::runtime_error("Cannot read configured file");
        }
        return bytes;
    }

    void atomic_write(const std::filesystem::path& path, const Bytes& bytes)
    {
        // Keep the temporary file beside its destination so replacement stays on the same filesystem.
        auto temporary = path;
        temporary += ".tmp-" + random_secret();
        try
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            if (!out || !out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()))
            {
                throw std::runtime_error("Cannot write state");
            }
            out.flush();
            if (!out)
            {
                throw std::runtime_error("Cannot flush state");
            }
            out.close();
            if (!out ||
                !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            {
                throw std::runtime_error("Cannot replace state");
            }
        }
        catch (...)
        {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            throw;
        }
    }

    Storage::Storage(const std::string& directory) : root_(directory)
    {
        for (const auto* kind : {"memberships", "profiles", "complaints"})
        {
            std::filesystem::create_directories(root_ / kind);
        }
    }

    std::filesystem::path Storage::user_path(const std::string& name, const char* kind) const
    {
        if (!valid_name(name))
        {
            throw std::runtime_error("Invalid account identity");
        }
        std::string filename;
        const char* digits = "0123456789abcdef";
        // Hex filenames preserve normalized identity without interpreting names as paths or Windows device names.
        for (unsigned char character : key(name))
        {
            filename += digits[character >> 4];
            filename += digits[character & 15];
        }
        return root_ / kind / (filename + ".bin");
    }

    Fields Storage::membership(const std::string& user) const
    {
        auto path = user_path(user, "memberships");
        Fields fields;
        if (std::filesystem::exists(path))
        {
            fields = decode_fields(FindMessageDefinition(SierraNW, 314)->format, read_file(path, 8192));
        }
        else
        {
            fields.assign(11, std::string());
            fields.insert(fields.end(), {uint16_t(0), uint16_t(0), int32_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
                                         uint8_t(0), uint8_t(0), uint8_t(0)});
        }
        fields[0] = user;
        fields[2] = std::string();
        return fields;
    }

    void Storage::save_membership(const std::string& user, const Fields& input, unsigned message)
    {
        // Signup 620 has 12 strings and edit 622 has 14; both project onto 314's 11 strings plus nine scalars.
        auto strings = message == 620 ? 12u : 14u;
        if (input.size() != strings + 9)
        {
            throw std::runtime_error("Invalid membership fields");
        }
        for (unsigned fieldIndex = 0; fieldIndex < strings; ++fieldIndex)
        {
            if (std::get<std::string>(input[fieldIndex]).size() > 256)
            {
                throw std::runtime_error("Membership field too long");
            }
        }
        Fields fields(input.begin(), input.begin() + 11);
        fields.insert(fields.end(), input.begin() + strings, input.end());
        fields[0] = user;
        fields[2] = std::string(); // Neither a claimed identity nor a password/placeholder is persisted here.
        atomic_write(user_path(user, "memberships"),
                     encode_fields(FindMessageDefinition(SierraNW, 314)->format, fields));
    }

    Fields Storage::profile(const std::string& user) const
    {
        auto path = user_path(user, "profiles");
        Fields fields{user, user, uint8_t(0), std::string(), std::string()};
        if (std::filesystem::exists(path))
        {
            fields = decode_fields("%s %s %c %s %s", read_file(path, 8192));
        }
        fields[0] = user;
        fields[1] = user;
        fields[2] = uint8_t(std::get<uint8_t>(fields[2]) & 0x3f);
        return fields;
    }

    void Storage::save_profile(const std::string& user, const Fields& input)
    {
        // Client-supplied privilege bits are never trusted; server moderator roles live in a separate account list.
        auto fields = input;
        fields[0] = user;
        fields[1] = user;
        fields[2] = uint8_t(std::get<uint8_t>(fields[2]) & 0x3f);
        if (std::get<std::string>(fields.at(3)).size() > 2000 || std::get<std::string>(fields.at(4)).size() > 2000)
        {
            throw std::runtime_error("Profile too long");
        }
        atomic_write(user_path(user, "profiles"), encode_fields("%s %s %c %s %s", fields));
    }

    void Storage::complaint(const std::string& user, const Fields& fields)
    {
        if (std::get<std::string>(fields.at(0)).empty() || std::get<std::string>(fields[0]).size() > 64 ||
            std::get<std::string>(fields.at(1)).empty())
        {
            throw std::runtime_error("Invalid complaint");
        }
        atomic_write(root_ / "complaints" / (random_secret() + ".bin"),
                     encode_fields("%s %s %s %s", {utc_timestamp(), user, fields[0], fields[1]}));
    }
} // namespace sigs::server
