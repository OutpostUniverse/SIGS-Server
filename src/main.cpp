#include "Server.h"
#include <windows.h>
#include <iostream>
#include <cmath>

static std::string password()
{
    auto input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    bool console = GetConsoleMode(input, &mode) != 0;
    // Hide interactive entry while preserving redirected stdin for unattended account provisioning.
    if (console)
    {
        SetConsoleMode(input, mode & ~ENABLE_ECHO_INPUT);
    }
    std::string value;
    std::getline(std::cin, value);
    if (console)
    {
        SetConsoleMode(input, mode);
        std::cout << '\n';
    }
    return value;
}

int main(int argc, char** argv)
{
    try
    {
        sigs::server::Config config;
        std::string add_user;
        bool reset = false, coverage = false;
        for (int argumentIndex = 1; argumentIndex < argc; ++argumentIndex)
        {
            std::string arg = argv[argumentIndex];
            auto value = [&]()
            {
                if (++argumentIndex == argc)
                {
                    throw std::runtime_error("Missing option value");
                }
                return std::string(argv[argumentIndex]);
            };
            if (arg == "--bind")
            {
                config.bind = value();
            }
            else if (arg == "--advertise")
            {
                config.advertise = value();
            }
            else if (arg == "--accounts")
            {
                config.accounts = value();
            }
            else if (arg == "--add-user")
            {
                add_user = value();
            }
            else if (arg == "--reset-user")
            {
                add_user = value();
                reset = true;
            }
            else if (arg == "--data")
            {
                config.data = value();
            }
            else if (arg == "--updates")
            {
                config.updates = value();
            }
            else if (arg == "--moderators")
            {
                config.moderators = value();
            }
            else if (arg == "--news")
            {
                config.news = value();
            }
            else if (arg == "--enable-signup")
            {
                config.signup = true;
            }
            else if (arg == "--message-coverage")
            {
                coverage = true;
            }
            else if (arg == "--required-dll-version")
            {
                auto text = value();
                size_t used = 0;
                config.required_dll = std::stof(text, &used);
                if (used != text.size() || !std::isfinite(config.required_dll) || config.required_dll <= 0)
                {
                    throw std::runtime_error("Expected positive binary32 DLL version value");
                }
            }
            else if (arg == "--base-port")
            {
                auto text = value();
                size_t used = 0;
                config.port = std::stoi(text, &used);
                if (used != text.size() || config.port < 1 || config.port > 65000)
                {
                    throw std::runtime_error("Invalid base port");
                }
            }
            else if (arg == "--permissive-auth")
            {
                config.permissive = true;
            }
            else if (arg == "--help")
            {
                std::cout << "sigs-server [--bind IPv4] [--advertise IPv4] [--base-port 7800]\n"
                             "            [--accounts FILE] [--permissive-auth]\n"
                             "            [--data DIRECTORY] [--updates MANIFEST] [--moderators FILE] [--news FILE]\n"
                             "            [--enable-signup] [--required-dll-version FLOAT]\n"
                             "sigs-server --accounts FILE --add-user NAME|--reset-user NAME (password twice on stdin)\n"
                             "sigs-server --message-coverage\n";
                return 0;
            }
            else
            {
                throw std::runtime_error("Unknown option: " + arg);
            }
        }
        if (coverage)
        {
            size_t count = 0;
            auto definitions = sigs::core::GetMessageDefinitions(sigs::core::SierraNW, &count);
            std::cout << "ID\tStatus\tFormat\tRegistry label\n";
            for (size_t definitionIndex = 0; definitionIndex < count; ++definitionIndex)
            {
                const auto& definition = definitions[definitionIndex];
                std::cout << definition.id << '\t' << sigs::server::message_support(definition.id) << '\t'
                          << definition.format << '\t' << definition.descriptive_name << '\n';
            }
            return 0;
        }
        if (!add_user.empty())
        {
            // Administrative account changes complete without opening network listeners or starting the event loop.
            std::cout << "Password: " << std::flush;
            auto enteredPassword = password();
            std::cout << "Confirm password: " << std::flush;
            auto confirmedPassword = password();
            if (enteredPassword != confirmedPassword)
            {
                throw std::runtime_error("Passwords do not match");
            }
            if (reset)
            {
                sigs::server::Accounts::reset(config.accounts, add_user, enteredPassword);
            }
            else
            {
                sigs::server::Accounts::add(config.accounts, add_user, enteredPassword);
            }
            SecureZeroMemory(enteredPassword.data(), enteredPassword.size());
            SecureZeroMemory(confirmedPassword.data(), confirmedPassword.size());
            std::cout << (reset ? "Password reset\n" : "Account created\n");
            return 0;
        }
        if (config.permissive)
        {
            std::cerr << "Permissive authentication enabled: use only in an isolated test network\n";
        }
        sigs::server::Server server(std::move(config));
        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
