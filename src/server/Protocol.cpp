#include "Server.h"
#include <iostream>

namespace sigs::server
{
    const char* message_support(unsigned id)
    {
        // This is implementation coverage, not a routing or authorization table; service handlers still enforce
        // context.
        switch (id)
        {
        case 1:
        case 102:
        case 104:
        case 106:
        case 107:
        case 109:
        case 111:
        case 601:
        case 602:
        case 603:
        case 605:
        case 606:
        case 607:
        case 608:
        case 609:
        case 612:
        case 615:
        case 616:
        case 617:
        case 619:
        case 620:
        case 622:
        case 626:
        case 627:
        case 631:
        case 632:
        case 634:
        case 701:
        case 702:
        case 703:
        case 704:
        case 801:
        case 802:
        case 804:
            return "handled";
        case 101:
            return "codec-only";
        case 103:
        case 110:
        case 201:
        case 202:
        case 207:
        case 301:
        case 302:
        case 303:
        case 304:
        case 306:
        case 308:
        case 309:
        case 310:
        case 311:
        case 313:
        case 314:
        case 315:
        case 316:
        case 401:
        case 402:
        case 403:
        case 404:
        case 405:
        case 406:
        case 407:
        case 408:
        case 409:
        case 410:
        case 411:
        case 412:
        case 803:
        case 901:
        case 902:
        case 903:
        case 904:
        case 905:
        case 906:
        case 907:
            return "outbound";
        default:
            return "unresolved";
        }
    }

    bool Server::moderator(const Client& client) const
    {
        // Knowing an administrator message ID or setting a profile flag never grants this role.
        return client.service == Service::Lobby && !client.user.empty() && !client.closing &&
               moderators_.count(key(client.user));
    }

    void Server::moderation(Client& client, unsigned messageId, const Fields& fields)
    {
        if (!moderator(client))
        {
            std::cerr << "Rejected unauthorized moderation id=" << messageId << " connection=" << client.id << '\n';
            return;
        }
        auto text = messageId == 801   ? std::get<std::string>(fields[0])
                    : messageId == 802 ? std::get<std::string>(fields[1])
                                       : std::string();
        if ((messageId == 801 || messageId == 802) && (text.empty() || text.size() > 2000))
        {
            return;
        }
        if (messageId == 801)
        {
            for (auto& [connectionId, target] : clients_)
            {
                if (target.service == Service::Lobby && !target.user.empty())
                {
                    send(target, 801, {text});
                }
            }
        }
        else
        {
            auto recipient = key(std::get<std::string>(fields[0]));
            for (auto& [connectionId, target] : clients_)
            {
                if (target.service == Service::Lobby && !target.user.empty() && key(target.user) == recipient)
                {
                    // Client 802 is recipient/text; the recipient's receive handler expects text/unused auxiliary
                    // string.
                    if (messageId == 802)
                    {
                        send(target, 802, {text, std::string()});
                    }
                    else if (messageId == 804 && target.id != client.id)
                    {
                        // Closing owned channels releases room/game state through the normal deferred cleanup path.
                        target.closing = true;
                        for (auto& [ownedConnectionId, child] : clients_)
                        {
                            if (child.owner == target.id)
                            {
                                child.closing = true;
                            }
                        }
                    }
                }
            }
        }
    }
} // namespace sigs::server
