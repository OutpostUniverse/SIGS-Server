# Sierra Internet Gaming System (SIGS) 3.0.3.0 C++ server

A Windows C++17 server reconstruction for Sierra Internet Gaming System (SIGS) 3.0.3.0 and its SNWVALID
3.0.3.1 companion. The default game is Outpost 2 (`OUTPOST2`).

**45 message entries remain unresolved.**
See the [message audit](docs/message-coverage.md) for current coverage.

## Build and run

Install Visual Studio with Desktop development with C++, CMake and Ninja.
The build script finds the installation automatically; Build Tools is also supported.
From the project folder:

```powershell
.\build.cmd
.\build\sigs-server.exe --add-user Alice
.\build\sigs-server.exe
```

`build.cmd` builds and runs the tests. Python is needed for service and research
tests, but not to run the server. Create accounts while the server is stopped.
Use `--help` for options.
Research scripts, evidence and analysis libraries are excluded from new Git tracking.
See [Git exclusions and fresh-clone testing](docs/version-control.md).

For LAN access, replace the example address with this machine's IPv4 address:

```powershell
.\build\sigs-server.exe --bind 0.0.0.0 --advertise 192.168.0.127
```

Point the client's validation endpoint to that address on TCP 7800.
Use a trusted LAN or VPN: the original protocol sends traffic in plaintext.

## Documentation

- [Setup, ports, accounts and compatibility limits](docs/server-guide.md)
- [Message services and operator configuration](docs/message-services.md)
- [Update configuration](docs/updates.md)
- [Project structure](docs/project-structure.md) and [architecture](docs/architecture.md)
- [DLL investigation](docs/message-investigation.md)
