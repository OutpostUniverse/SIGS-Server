# Added message services

## Profiles and reports

109 gaming profiles are saved under `data/profiles`, restored on room entry,
and broadcast through 111 without changing gameplay endpoints. Client-provided
privilege bits are masked; flags never grant server roles. 622 membership edits
are tied to the authenticated owner and persisted under `data/memberships`.
The 314 reply fixes the identity and blanks the password/placeholder slot;
opaque scalars are retained without inventing enum meanings.
Successful 109 edits receive 313(1); successful 622 edits receive
315(1, authenticated name) alongside the refreshed 314. Invalid edits receive
a negative result. These result IDs and status-1 success branches were recovered
from the original lobby handlers.

632(username) requests public information and receives 316 for an existing
account: username, city, state, country; HasCD/HasMike shorts; AgeGroup,
GenderGroup, ModemSpeedGroup, MonitorGroup, RamCodeGroup and ProcessorGroup bytes.
The projection excludes real name, address, password and email. Strings are
bounded for the SDK's tPublicInfo buffers. Nonexistent names receive no invented
not-found response; its original error path has not been recovered.

631 complaints are stored under `data/complaints` as four NUL-terminated fields:
UTC timestamp, authenticated submitter, original string 0, original string 1.
One per connection per 30 seconds is accepted, without an invented wire reply.
These files contain private user content. Keep the data directory private and
include it in backups. `--data DIRECTORY` chooses its location.

## Administrator messages

`--moderators moderators.txt` loads one quoted existing account name per line:

```text
"Alice"
```

Only those authenticated accounts can broadcast 801, send private 802 notices,
or request removal through 804. 802 recipient/text is converted to the original
incoming text/empty-auxiliary field order. 804 closes the target lobby and owned
room/game connections. Neither a profile bit nor a message ID confers privilege.
Restart to apply role-list edits. This account-role and removal policy is an
emulator choice; complete historical authorization behavior is unrecovered.

`--news news.txt` loads records like:

```text
7 "Welcome" "A short plain-text bulletin"
```

Each 803 record is sent on successful login. Use positive increasing revisions;
the client owns its per-category suppression preferences. Bodies are limited to
2000 bytes and records to 16. Text is literal, without HTML interpretation or
backslash-newline expansion. The original client may route 801/802 to chat or a
game callback, rather than a popup; 803 has its own News & Info dialog.

## Registration and version rejection

`--enable-signup` enables 620 registration and 302 replies. The original client
supplies a literal PASSWORD placeholder rather than the user's usable password.
The server saves its membership and creates a random, unrecoverable hashed
secret. The operator must provision a password, while the server is stopped:

```powershell
.\build\sigs-server.exe --reset-user NewMember
```

Restart before login. No email is sent; the success response says so. This
implements local provisioning, not Sierra's unrecovered email service. Signup
is disabled by default. Do not enable it on an untrusted network without
external abuse controls. 621 password-change field roles and reply remain
unresolved; accepting two arbitrary strings as old/new passwords would invent
behavior and could overwrite credentials incorrectly.

`--required-dll-version 3.5` makes signup receive 301 with that exact positive
binary32 value. Dotted SIGS version mapping is unknown; this option intentionally
accepts a float, never a guessed conversion from `3.0.3.0`.

## Legacy rooms and state

603 accepts a room password of up to 128 bytes. 306 advertises it to every
authenticated lobby client. The original client compares the password locally
before sending 602; no password is included in 602. This reproduces the legacy
client gate and cannot enforce secret server-side password admission. Never
reuse account passwords. Admission grants expire after 30 seconds, are single
use, and bind the lobby owner, peer address and room.

An occupied custom room retires after its last room client disconnects. 310
reports population zero, then 309 removes the listing for unaffected clients.
Departing users receive 309 only after a 107 confirms entry to another room,
avoiding the original client's active-room exit branch. Names with deferred
removals remain reserved. The permanent Main Lobby is never retired.

627 accepts a matching game name from its current host. The SDK establishes
integer 0 as a parameter and integer 1 as its value. Parameter 1 selects join
permission: 1 closes joining, 2 allows joining without approval, 5 requires owner
approval. These modes now affect 608/609. Modes 3/4, voting modes 6/7, watch
permissions and in-progress joins remain unimplemented. Other values are stored
without changing admission policy. Parameter 2 is invite permission; its SDK
implemented value 3 matches the existing member-invite policy. See the
[investigation](message-investigation.md) for exact evidence and remaining limits.

101 has a recovered warning layout but no established original connectivity
probe protocol. Its codec exists; the server does not substitute it for unrelated
errors. This is tracked separately from implemented outgoing messages.

## Compound framing

Message 1 wraps a length-prefixed binary block of consecutive inner messages
(12-byte header plus payload, without their TCP outer length words). They pass
through the same role and identity checks as ordinary packets. Limits are four
levels of nesting and 128 nodes per top-level packet. Truncated headers, invalid
lengths and excess nesting close that connection. Replies remain independent
frames. This is a recovered container, not arbitrary compression/flag transforms.
