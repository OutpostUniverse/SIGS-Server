# Server setup and compatibility

## Build and run

Install Visual Studio with Desktop development with C++, CMake and Ninja.
`build.cmd` discovers the installed C++ toolchain automatically, including Build Tools.
When copying the project, you can leave out the generated `build/` directory.
If it was included, the script detects the old cache location, preserves the
directory as `build-backup-<timestamp>-<suffix>`, and generates a fresh build.
Existing backups are kept and excluded from version control. If you change
toolchains without moving the project, rename `build/` manually before rebuilding.
From this folder:

```powershell
.\build.cmd
.\build\sigs-server.exe --add-user Alice
.\build\sigs-server.exe
```

The account command prompts twice with console echo disabled. Account files
contain random salts and PBKDF2-HMAC-SHA256 hashes (200,000 iterations), never
plaintext passwords. Create accounts while the server is stopped; restart to
load changes. Back up `accounts.txt` and restrict its Windows file permissions.
Names are case insensitive and must contain 1–32 printable non-space ASCII
characters, excluding quotes and backslashes. Passwords contain 1–128 bytes.
Use `--reset-user Alice` to replace an existing password while the server is
stopped. Account writes use atomic replacement.

For a LAN, choose this machine's reachable IPv4 address:

```powershell
.\build\sigs-server.exe --bind 0.0.0.0 --advertise 192.168.0.127
```

Configure the original client's validation endpoint for that address, TCP 7800,
using the existing SIGS endpoint configuration/launcher. The server advertises
the subsequent endpoints; it does not patch client files or redirect DNS.
Use separate accounts for each player. Version 3.0.3.0 is the sole wire profile;
Extensions from later protocol versions are not selected by this server.

Options: `--bind IPv4`, `--advertise IPv4`, `--base-port NUMBER`,
`--accounts PATH`, `--data DIRECTORY`, `--permissive-auth`, `--help`. The base port defaults to
7800; changing it shifts every endpoint together. Bind and advertised addresses
default to loopback. Permissive mode accepts arbitrary valid names without
checking passwords; use it only for isolated protocol tests.

## Service layout

| Service | Default endpoint | Behavior |
|---|---|---|
| Validation | TCP 7800 | Version handoff, no-update replies, configured update offers/downloads |
| Lobby | TCP 7801 | Accounts, profiles, directory, admission, complaints, authorized notices/removal |
| Main room | TCP 7802 | Player records, chat, game listings, host approval, invitations |
| Game control | TCP 7803 | 102 → 103 transport handoff, readiness, host start |
| Game data | TCP 7804 | Separate 601/207/107 handshake; experimental 703/704 TCP forwarding |
| UDP reachability | UDP 7805 | 634/110 authorization followed by bounded 102 echo |
| Additional rooms | TCP 7810–7812, 7813–7815, … | Separate room/control/data triple per room |

Only open the required endpoints on your firewall. The reconstruction keeps
separate sockets and roles, including the different meanings of 601 in the
lobby and game-data service. Game start sends 405 on each participant's room
connection, after both game transport connections finish initialization.
Peer records preserve the reported gameplay port and LAN address; different
public-IP recipients receive the observed public IP where applicable.

Original client traffic is plaintext. Internal handoffs bind a username to its
authenticated lobby session, source IP, room admission, and game membership.
The original wire protocol supplies no cryptographic handoff token: someone
sharing the same source IP can impersonate a pending handoff. Use a trusted LAN
or VPN. This implementation does not add unrecognized authentication messages.

## Verification and compatibility boundaries

`build.cmd` builds the executable and runs CTest. With Python available, CTest
also runs independent standard-library network tests against a temporary server
with real hashed accounts. Tests cover literal frames, fragmented/coalesced
requests, malformed input, login rejection, authenticated handoffs, room entry,
chat sender identity, UDP probe gating, host-approved joins, invitations,
two-player transport readiness and start, binary relay, cleanup, distinct games,
custom-room endpoints, and duplicate login rejection. The inherited core tests
exercise serialization, registry lookup, stream errors, and real TCP sockets.
Python is only needed for tests; the server has no Python runtime dependency.
Research scripts, saved evidence and analysis libraries are ignored by Git.
CMake automatically omits research tests when those required local files are absent; see
[Git exclusions and fresh-clone testing](version-control.md).
Feature tests also cover persisted profiles/membership across restarts,
complaint storage, 801/802/804 permissions, 803 news, registration and offline
password provisioning, 301 float encoding, legacy password rooms, deferred room
removal, 903–906 transfer, checksums, and redacted unresolved-message diagnostics.

See [message services and operator configuration](message-services.md),
[update configuration](updates.md), and the full
[118-message audit](message-coverage.md). The executable's
`--message-coverage` command prints the audit from the actual compiled registry.

**The unmodified SIGS client has not been run against this new server.** Passing
wire tests establishes the implemented message sequences, not complete game
compatibility. The next live checkpoint is two original Outpost 2 clients
entering a room, creating/joining a game and reaching Game Settings together.

Current boundaries:

- Update transfers are opt-in and checksum checked; server code never installs
  or executes packages. Original-client installation is unverified. No-match
  known game checks receive no-update; unknown games receive 902. Unconfigured
  alternate SIGS versions close without a guessed reply.
- No email delivery, billing, or recovered password-change behavior for 621.
  Optional signup uses local operator provisioning, not email verification.
- **45 registry entries remain unresolved with no complete semantic handler.** Knowing
  their layout does not establish purpose, direction, routing or replies.
  Diagnostics record only ID, connection, status and payload size, once per
  connection; fields and credentials are never logged and no reply is guessed.
- Legacy room passwords are advertised to lobby clients for local comparison.
  This reproduces the original client gate but is not secret password admission.
- Room and game state are in memory. One active lobby and room session per
  account; up to 128 TCP connections, 16 rooms and 32 games per room. Names and
  descriptions have validation limits; per-client output is limited to 1 MiB.
- Room and game names allow spaces, with at least one non-space character and
  a maximum of 32 printable ASCII characters, excluding quotes and backslashes.
  Account-name rules remain stricter and do not allow spaces.
- Games support 2–4 players in the initial 401 policy. The first remaining member
  owns host approval/start after a host leaves. Late joins after start are rejected.
- UDP 7805 tests reachability only. No server-side gameplay UDP relay, NAT
  traversal or tunnel is implemented. Original peer UDP still needs a usable
  network path. The UDP port in 103 is preserved as the historical transport
  endpoint but no gameplay UDP service is bound there.
- Binary 703/704 forwarding follows the research server's experimental adapter;
  its complete historical semantics remain unresolved.
- Modern 64-bit Windows server build; this is not an XP server binary or a
  drop-in SIERRANW.DLL. The original 32-bit client can use its TCP protocol.

See [architecture and evidence](architecture.md) for provenance and design.
The [follow-up DLL investigation](message-investigation.md) records recovered
compound framing, profile results, public-info queries and SDK permissions.
