# Project organization

```text
sigs-server-v3/
    CMakeLists.txt
    build.cmd
    src/
        main.cpp
        core/
        server/
    tests/
    docs/
        research/          (local and ignored)
    tools/
    references/
        binaries/
    third_party/           (local and ignored)
        research/
    build/                 (generated and ignored)
    build-backup-*/         (preserved after relocation and ignored)
```

## Source code

`src/core/` builds the `sigscore` library in namespace `sigs::core`. It owns
message definitions, field serialization, framing, TCP sockets and generic
connection/dispatch helpers. Historical filenames are retained for traceability.
`SIGSCore.h` and `MsgDef.h` expose its interfaces; `SocketInternal.h` is an
implementation header used only by the core's socket code.

`src/main.cpp` reads options and starts the `sigs-server` executable.
`src/server/` implements the application in namespace `sigs::server`.
`Server.cpp` owns the event
loop and connections; `Services.cpp` implements service message handlers;
`Protocol.cpp` reports implementation coverage and handles moderation policy.
Accounts, persistent records and update packages live in their respective
`Accounts`, `Storage` and `Updates` files. Headers stay beside their implementations.
The server depends on the core; the core does not depend on server services.

Keep these directories flat while the responsibilities remain easy to find.
Add a subdirectory when a feature has several related files, rather than
creating a directory for every class. A separate public `include/` tree is
unnecessary until the core is distributed as an installable library.

## Tests and development tools

`tests/core.cpp` exercises the core library. `tests/integration.py` runs an
independent wire client against a temporary server, keeping its accounts and
data in a temporary directory. The local, ignored `tests/research.py` checks
recovered DLL mappings against the reference binaries. CMake omits this test
when its local scripts, libraries or binaries are missing; see
[fresh-clone testing](version-control.md#fresh-clones-and-research-tests).
`tests/build_layout.py` checks relocation detection and preservation of old builds.

The local, ignored `tools/research.py` is a read-only developer tool, not a server dependency.
It reads the DLL in `references/binaries/` and uses local pefile/Capstone
libraries in the ignored `third_party/research/` directory. These files must be
restored separately in a fresh clone. The local Capstone library targets Windows x64, matching
the server development platform. Commands and evidence boundaries are
documented in [message investigation](message-investigation.md).

## Documentation, build files and runtime data

Keep operator guides, architecture and protocol explanations directly in `docs/`.
Put raw evidence and historical records in `docs/research/`, linking them from
the relevant guide. Keep original reference binaries in `references/binaries/`
and unmodified analysis dependencies with their licenses in `third_party/research/`.
The evidence and dependency directories are ignored; ordinary guides remain
part of the published source. See [Git exclusions](version-control.md) for the
complete list and the treatment of already tracked files.

The root `CMakeLists.txt` defines the library, executable and tests; `build.cmd`
configures, builds and tests them on Windows. Before configuration,
`tools/prepare-build.cmake` preserves caches copied from another location.
The formatting configuration and project README also belong at the root.
Generated outputs and disposable local
scripts belong in the ignored `build/` directory.

By default, running from the project root creates `accounts.txt` and `data/`
there; both are ignored. These are operator-owned runtime files, separate from
source and tests. Use `--accounts` and `--data` to choose another location.
