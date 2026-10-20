# Git exclusions and local files

The root `.gitignore` keeps the following local files out of new Git tracking:

| Pattern | Purpose |
|---|---|
| `/build/`, `/build-backup-*/`, `/out/`, `/cmake-build-*/` | Generated builds, preserved builds and alternate build outputs |
| `/.vs/` | Local Visual Studio state |
| `/accounts.txt`, `/accounts.txt.tmp-*`, `/data/` | Account records, temporary account writes, profiles and complaints |
| `/moderators.txt`, `/news.txt`, `/updates.txt`, `/packages/` | Local operator settings and update packages |
| `/docs/research/` | Saved research evidence and historical validation records |
| `/third_party/` | Local analysis libraries, package metadata and licenses |
| `research.py` | Any file with this name, including `tools/research.py` and `tests/research.py` |
| `*.log`, `*.dmp` | Logs and crash dumps |
| `.env`, `.env.*` | Local environment and secret files; `.env.example` is allowed |
| `__pycache__/`, `*.py[cod]`, `Thumbs.db`, `Desktop.ini` | Python caches and Windows metadata |

Root-relative patterns only cover the named locations. If you configure accounts,
data, operator settings or packages elsewhere in the repository, add those paths
to your local exclusions too. Use separately named example files for shareable
configuration; never put credentials in `.env.example`.

`references/binaries/` is **not ignored** by these rules. Neither are the C++
sources, build scripts, normal documentation or non-research tests.

## Fresh clones and research tests

The server and its core, service and build-layout tests do not require the ignored
research files. Original-DLL analysis requires local copies of both research
scripts, the libraries under `third_party/research/`, and the reference binaries.
Saved evidence under `docs/research/` is also local; filenames mentioned in the
guides may therefore be absent from a fresh clone.

CMake registers the `research` test only when Python and its local research
scripts, library inputs and reference binaries are present. Otherwise it prints
a message during configuration and leaves that test out of the suite.
The normal build command works for both a fresh clone and a complete local copy:

```powershell
.\build.cmd
```

Core tests still run, along with service and build-layout tests when Python is
available. Restore the local research files and rerun `build.cmd` to enable the
research test. Saved evidence under `docs/research/` is not needed to run it.
If present inputs are corrupt or incompatible, research failures still surface
normally rather than being silently skipped.

## Already tracked files

`.gitignore` does not untrack files already committed or staged, and does not
remove anything from commit history. Check `git status --ignored` and
`git ls-files -ci --exclude-standard` before publishing. The latter lists tracked
files that now match ignore rules. Removing files from the index with
`git rm --cached` preserves the working copies; use `-r` for directories.
