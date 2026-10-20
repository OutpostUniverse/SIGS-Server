# Reference binaries

`binaries/` contains the supplied original client DLLs used for protocol research:

| File | SHA-256 |
|---|---|
| SIERRANW.DLL | f41ae063d7197eda7c012ef3c32810f62edcbd4d72f79a8ec1e5551cf77003c3 |
| snwvalid.dll | 863c8fe7d44a28ca9f955235fd92bc4d31f496caf980205a4fd07028fdeec550 |

These are historical reference inputs, separate from the new C++ server source.
The analysis tool reads SIERRANW.DLL as data and checks its hash before parsing;
the research tests also check the validation DLL's hash. Neither DLL is loaded
or executed by the tool, tests or server, and neither is linked into the server.

Run `python tools/research.py tables` from the project root to inspect callback
mappings. No external research checkout is required.
The research scripts and `third_party/` libraries are ignored by Git and may
need to be restored locally. The reference DLL directory itself is not ignored.
See [Git exclusions](../docs/version-control.md).
