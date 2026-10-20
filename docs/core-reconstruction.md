# SIGS C++ core reconstruction

Originally reconstructed on 26 September 2026. This is a new, buildable C++17 implementation organized
under the 13 requested historical filenames. It is NOT decompiled original
Sierra source, binary-compatible SIERRANW, or a complete replacement for the
Python SIGS server on its own. The library now lives in `src/core/`, with
server services in `src/server/` and its tests in `tests/core.cpp`.
Revision strings prove filenames existed, not their exact
classes, responsibilities, algorithms or APIs. File responsibilities below
are explicit replacement design choices.

## Implemented files

| File | Replacement responsibility | Evidence boundary |
|---|---|---|
| SNString.cpp | NUL-terminated byte strings | Working wire profile; no guessed Unicode conversion |
| SNComMgr.cpp | Explicitly owned connections, send/poll/remove | New API; no original scheduler recovered |
| MsgHdr.cpp | Length, ID, flags and frame validation | Matches Python framing and literal vectors |
| LstBlck.cpp | Incremental fragmented/coalesced TCP frame reader | Replacement stream accumulator, not recovered Sierra block-list internals |
| FrmtCtrl.cpp | Format token parsing and unsupported-token rejection | Recovered format strings |
| SNGen.cpp | Checked big-endian integer helpers | Wire encoding evidence |
| SNDate.cpp | UTC log timestamp utility | Modern replacement ONLY; original date class and %t wire encoding unresolved |
| SNSocTCP.cpp | RAII TCP connect/listen/accept/send/receive with deadlines | Modern nonblocking Winsock/POSIX design |
| SNComSoc.cpp | Packet connection wrapper and terminal error cleanup | Replacement lifecycle policy |
| Format.cpp | Typed field encoding/decoding | %s, %i, %l, %c; statically traced %f; experimental length-prefixed %b |
| SNSockIP.cpp | IPv4 endpoints, socket runtime, readiness waits | New transport support; no ICMP probe implementation |
| ProcessMsg.cpp | Registered callback dispatch | Does NOT recreate original lobby/room/game handlers |
| MsgDef.cpp | Both 118-entry recovered DLL registries | ID/formats extracted; descriptive names are project labels |

Headers are SIGSCore.h, SocketInternal.h and MsgDef.h. The original import
list (WSACancelBlockingCall, ICMP.DLL etc.) is not a specification of APIs to
copy. This version uses modern socket calls; DNS names, UDP transport, ICMP,
legacy blocking-call cancellation and original binary exports are not supplied.

## Wire support

Strings are byte-preserving, NUL-terminated, with embedded NUL rejected.
Callers must supply their chosen legacy byte encoding; no automatic UTF-8 or
Latin-1 conversion is performed. %i is signed 32-bit, %l unsigned 16-bit,
%c unsigned 8-bit, all multibyte fields big-endian. %f uses IEEE binary32,
matching the traced accessor. %b uses the project's experimental BE32-length
adapter; this does not prove every original binary-block mode behaves that way.
%t, %lf and all other unsupported tokens throw. Unknown messages can remain
opaque Packet payloads. Generic dispatch does not invent their behavior.

Framing uses equal outer/inner lengths of 12 + payload size, with flags
preserved. The 1 MiB frame limit is project policy. A reader retains partial
frames, accepts coalesced frames, and distinguishes clean EOF from truncation.
This does not implement multi-message blocks or alternate flag transformations.

## Build and tests

Use a modern C++17 compiler; this is NOT an XP-targeted binary build.
From the project root on Windows:

```text
.\build.cmd
```

The build script uses an existing Visual Studio developer prompt,
or the Visual Studio 2026 Community path available on the development machine.
Adjust that fallback path on other systems. Winsock links with ws2_32.lib.
SocketRuntime must outlive all sockets. POSIX code is provided but not compiled
or executed in this Windows validation run.

`tests/core.cpp` uses fixed wire vectors as well as round trips. It covers both registries,
field encoding, rejected unsupported types and malformed fields, frame limits,
fragmented/coalesced input, callback dispatch, real localhost TCP traffic,
clean/truncated EOF, timeouts, and connection-manager callback mutation guards.
The core tests do not validate the updater. Separate service tests exercise
update transfers without executing downloaded packages.

## API and lifecycle rules

The library is single-threaded per object: callers must serialize access.
CommunicationManager callbacks must queue sends or connection changes until
poll returns; reentrant manager operations throw to avoid invalidating a live
connection. Callback exceptions close and remove the connection. A poll
socket timeout is terminal, not a heartbeat implementation. Configure suitable
deadlines and implement heartbeat/state policy above this core.

Example:

```cpp
#include "SIGSCore.h"
using namespace sigs::core;
Bytes wire = encode_packet({609, encode_fields("%s %i", {
    std::string("Alice"), std::int32_t(1)
})});
```

Accounts, permissions, room/game state, invitations and update transfers belong
to `src/server/`. See [the project README](../README.md) for implemented behavior
and compatibility limits. Keep the Python implementation and supplied DLLs as
independent references.

## Provenance

Local files under the ignored `docs/research/` directory include
`registry-evidence.json` (hashes and registration/format addresses),
`catalog-differences.json` (registry-only IDs) and `core-validation.txt`
(the original standalone validation record). They may be absent from fresh clones;
see [Git exclusions](version-control.md). Current validation uses the root build script,
with research tests automatically omitted if their local inputs are missing.
The registry-only IDs' semantic identities remain unresolved. The included MsgDef labels are
from the earlier registry reconstruction and may lag newer protocol notes;
format strings are the authoritative recovered content. No synced source
material or supplied binary was changed.
