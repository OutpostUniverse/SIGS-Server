#include "Accounts.h"
#include "Storage.h"
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <sstream>

namespace sigs::server
{
    std::string key(std::string text)
    {
        // Account identity follows ASCII case folding, independent of the server machine's locale.
        for (auto& character : text)
        {
            if (character >= 'A' && character <= 'Z')
            {
                character += 'a' - 'A';
            }
        }
        return text;
    }

    bool valid_name(const std::string& text)
    {
        if (text.empty() || text.size() > 32)
        {
            return false;
        }
        for (unsigned char character : text)
        {
            if (character < 33 || character > 126 || character == '"' || character == '\\')
            {
                return false;
            }
        }
        return true;
    }

    static std::string hex(const unsigned char* data, size_t byteCount)
    {
        static const char* digits = "0123456789abcdef";
        std::string output;
        for (size_t byteIndex = 0; byteIndex < byteCount; ++byteIndex)
        {
            output += digits[data[byteIndex] >> 4];
            output += digits[data[byteIndex] & 15];
        }
        return output;
    }

    static std::vector<unsigned char> unhex(const std::string& text, size_t count)
    {
        if (text.size() != count * 2)
        {
            throw std::runtime_error("Invalid account record");
        }
        std::vector<unsigned char> output;
        auto nibble = [](char character) -> unsigned char
        {
            if (character >= '0' && character <= '9')
            {
                return character - '0';
            }
            if (character >= 'a' && character <= 'f')
            {
                return character - 'a' + 10;
            }
            throw std::runtime_error("Invalid account hex");
        };
        for (size_t byteIndex = 0; byteIndex < text.size(); byteIndex += 2)
        {
            output.push_back((nibble(text[byteIndex]) << 4) | nibble(text[byteIndex + 1]));
        }
        return output;
    }

    static std::string derive(const std::string& password, const std::string& salt)
    {
        auto bytes = unhex(salt, 16);
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
        {
            throw std::runtime_error("Cannot open password provider");
        }
        std::array<unsigned char, 32> hash{};
        auto status = BCryptDeriveKeyPBKDF2(algorithm, (PUCHAR)password.data(), static_cast<ULONG>(password.size()),
                                            bytes.data(), static_cast<ULONG>(bytes.size()), 200000, hash.data(),
                                            static_cast<ULONG>(hash.size()), 0);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        if (status < 0)
        {
            throw std::runtime_error("Cannot derive password");
        }
        return hex(hash.data(), hash.size());
    }

    std::string random_secret()
    {
        std::array<unsigned char, 24> bytes{};
        if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) <
            0)
        {
            throw std::runtime_error("Cannot generate random value");
        }
        return hex(bytes.data(), bytes.size());
    }

    std::string sha256(const std::vector<unsigned char>& bytes)
    {
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        {
            throw std::runtime_error("Cannot open SHA256 provider");
        }
        std::array<unsigned char, 32> hash{};
        auto status = BCryptHash(algorithm, nullptr, 0, const_cast<PUCHAR>(bytes.data()),
                                 static_cast<ULONG>(bytes.size()), hash.data(), static_cast<ULONG>(hash.size()));
        BCryptCloseAlgorithmProvider(algorithm, 0);
        if (status < 0)
        {
            throw std::runtime_error("Cannot hash package");
        }
        return hex(hash.data(), hash.size());
    }

    Accounts::Accounts(const std::string& path)
    {
        std::ifstream input(path);
        if (!input)
        {
            if (std::filesystem::exists(path))
            {
                throw std::runtime_error("Cannot read accounts file");
            }
            return;
        }
        std::string name, salt, hash;
        while (input >> std::quoted(name))
        {
            if (!(input >> salt >> hash) || !valid_name(name))
            {
                throw std::runtime_error("Invalid account record");
            }
            unhex(salt, 16);
            unhex(hash, 32);
            if (!entries_.emplace(key(name), std::make_pair(salt, hash)).second)
            {
                throw std::runtime_error("Duplicate account");
            }
        }
        if (!input.eof() && input.fail())
        {
            throw std::runtime_error("Invalid accounts file");
        }
    }

    bool Accounts::verify(const std::string& name, const std::string& password) const
    {
        if (!valid_name(name) || password.empty() || password.size() > 128)
        {
            return false;
        }
        auto accountIterator = entries_.find(key(name));
        // Unknown names still incur the password derivation cost instead of returning before hashing.
        const auto salt = accountIterator == entries_.end() ? std::string(32, '0') : accountIterator->second.first;
        auto actual = derive(password, salt);
        const auto expected = accountIterator == entries_.end() ? std::string(64, '0') : accountIterator->second.second;
        unsigned difference = 0;
        // Compare the complete digest without an early return on the first different byte.
        for (size_t byteIndex = 0; byteIndex < actual.size(); ++byteIndex)
        {
            difference |= actual[byteIndex] ^ expected[byteIndex];
        }
        return accountIterator != entries_.end() && difference == 0;
    }

    void Accounts::add(const std::string& path, const std::string& name, const std::string& password)
    {
        if (!valid_name(name) || password.empty() || password.size() > 128)
        {
            throw std::runtime_error("Invalid name or password length");
        }
        Accounts existing(path); // Reload before replacing the file so other persisted accounts are retained.
        if (existing.entries_.count(key(name)))
        {
            throw std::runtime_error("Account already exists");
        }
        std::array<unsigned char, 16> salt{};
        if (BCryptGenRandom(nullptr, salt.data(), static_cast<ULONG>(salt.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        {
            throw std::runtime_error("Cannot generate salt");
        }
        auto salt_text = hex(salt.data(), salt.size());
        auto hash = derive(password, salt_text);
        existing.entries_.emplace(key(name), std::make_pair(salt_text, hash));
        std::ostringstream output;
        for (auto& [user, record] : existing.entries_)
        {
            output << std::quoted(user) << ' ' << record.first << ' ' << record.second << '\n';
        }
        auto text = output.str();
        atomic_write(path, sigs::core::Bytes(text.begin(), text.end()));
    }

    void Accounts::reset(const std::string& path, const std::string& name, const std::string& password)
    {
        if (!valid_name(name) || password.empty() || password.size() > 128)
        {
            throw std::runtime_error("Invalid name or password length");
        }
        Accounts existing(path);
        auto accountIterator = existing.entries_.find(key(name));
        if (accountIterator == existing.entries_.end())
        {
            throw std::runtime_error("Account does not exist");
        }
        auto salt = random_secret().substr(0, 32);
        accountIterator->second = {salt, derive(password, salt)};
        std::ostringstream output;
        for (auto& [user, record] : existing.entries_)
        {
            output << std::quoted(user) << ' ' << record.first << ' ' << record.second << '\n';
        }
        auto text = output.str();
        atomic_write(path, sigs::core::Bytes(text.begin(), text.end()));
    }
} // namespace sigs::server
