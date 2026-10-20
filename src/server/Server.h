#pragma once
#include "SIGSCore.h"
#include "Accounts.h"
#include "Storage.h"
#include "Updates.h"
#include <deque>
#include <optional>
#include <set>

namespace sigs::server
{
    using namespace sigs::core;
    using Fields = std::vector<Value>;
    using Clock = std::chrono::steady_clock;
    enum class Service
    {
        Validation,
        Lobby,
        Room,
        Control,
        Data
    };

    // Ports are deployment choices; the five roles preserve the recovered client handoffs.
    struct Config
    {
        std::string bind = "127.0.0.1";
        std::string advertise = "127.0.0.1";
        std::string accounts = "accounts.txt";
        int port = 7800; // Base port; the remaining endpoints are allocated relative to it.
        bool permissive = false;
        std::string data = "data";
        std::string updates;
        std::string moderators;
        std::string news;
        bool signup = false;
        float required_dll = 0; // Explicit 301 binary32 value; dotted-version conversion is unrecovered.
    };

    struct Client
    {
        unsigned id;
        Service service;
        int room;
        TcpSocket socket{-1};
        FrameStream frames;
        std::string ip;     // Observed peer IPv4 address.
        std::string user;   // Canonical authenticated account name.
        unsigned owner = 0; // Lobby connection ID; prevents an old session from owning a new login's channels.
        int admitted = 0;   // One-use room admission grant.
        int attempts = 0;   // Authentication/registration attempts on this connection.
        // ready is transport handshake state; closing stops reads while already queued replies drain.
        bool ready = false;
        bool closing = false;
        bool pinged = false;
        Fields record; // 111 wire order; empty until this room connection has identified its player.
        std::deque<Bytes> output;
        size_t queued = 0; // Total unsent bytes across output frames.
        size_t offset = 0; // Position within the front output frame.
        Clock::time_point last = Clock::now();
        Clock::time_point login_attempt{};
        Clock::time_point probe_expires{}; // Lobby: admission deadline; room: authorized UDP probe deadline.
        int probe_replies = 0;
        // Stages: 0 version check, 1 game check, 2/3 component/game offer, 4 transfer, 5 game transfer complete.
        int validation_stage = 0;
        int update = -1; // Index in the immutable startup-loaded package list.
        size_t update_offset = 0;
        // Rooms the client may still consider active; gate potentially destructive 309 notices.
        std::set<int> room_risks;
        std::map<int, std::string> removals;
        std::set<unsigned> unsupported;
        Clock::time_point last_complaint{};
    };

    struct Game
    {
        Fields listing; // 403: name, four settings, control host/port; opaque settings retain their wire positions.
        // Room-connection IDs; first remaining member owns approval/start by server policy.
        std::vector<unsigned> members;
        std::map<unsigned, int> slots;
        bool started = false;
        int32_t reported_state = 0;
        int32_t reported_state_aux = 0;
        int join_permission = 5; // SDK modes implemented here: 1 closed, 2 open, 5 owner approval.
    };

    struct Room
    {
        int id;
        int port;
        int capacity;
        std::string name;
        std::string description;
        std::map<std::string, Game> games;
        std::string password; // Legacy 306 advertises this for a client-side check; it is not secret admission proof.
        bool used = false;    // Newly created empty rooms survive until their first occupant has come and gone.
    };

    struct Listener
    {
        Service service;
        int room;
        TcpSocket socket;
    };

    struct Pending
    {
        unsigned host;
        int room;
        std::string game;
        Clock::time_point expires;
        bool invite;
    };

    // One event-loop thread owns all state; handlers queue writes and defer connection erasure until traversal ends.
    class Server
    {
        Config config_;
        SocketRuntime runtime_; // Declared before sockets so Winsock remains available through their destruction.
        Accounts accounts_;
        Storage storage_;
        std::vector<Update> updates_;
        std::set<std::string> moderators_;
        std::vector<Fields> news_;
        std::vector<Listener> listeners_;
        TcpSocket probe_;
        std::map<unsigned, Client> clients_;
        std::map<int, Room> rooms_;
        std::map<unsigned, Pending> pending_;
        unsigned next_client_ = 1;
        int next_room_ = 2;
        void listen(Service service, int roomId, int port);
        void probe();
        void pump_update(Client& client);
        void retire_rooms();
        bool moderator(const Client& client) const;
        void moderation(Client& client, unsigned messageId, const Fields& fields);
        void send(Client& client, unsigned messageId, const Fields& fields = {});
        void broadcast(int roomId, unsigned messageId, const Fields& fields);
        void handle(Client& client, const Packet& packet, unsigned depth = 0, unsigned* budget = nullptr);
        void validation(Client& client, unsigned messageId, const Fields& fields);
        void lobby(Client& client, unsigned messageId, const Fields& fields);
        void room(Client& client, unsigned messageId, const Fields& fields);
        void transport(Client& client, unsigned messageId, const Fields& fields);
        Client* identify(const Client& client, const std::string& username);
        Client* player(const std::string& username, int roomId);
        Client* channel(const Client& client, Service service);
        Game* game(unsigned playerId, int roomId);
        void enter(Client& member, Game& targetGame);
        void leave(Client& member);
        void disconnect(Client& client);
        void population(int roomId);
        void listing(Client& client, const Room& roomEntry);
        void refresh(Client& subject);
        Fields record_for(const Client& subject, const Client& recipient) const;

    public:
        explicit Server(Config config);
        void run();
    };

    const char* message_support(unsigned);
} // namespace sigs::server
