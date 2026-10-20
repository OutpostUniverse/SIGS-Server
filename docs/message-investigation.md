# Follow-up SIGS 3.0.3.0 investigation

The original DLL was parsed as a PE file, not loaded or executed. The analysis
checks SHA-256 before reading its instructions:
`f41ae063d7197eda7c012ef3c32810f62edcbd4d72f79a8ec1e5551cf77003c3`.

## Findings implemented in the server

| ID | Recovered role | Evidence |
|---|---|---|
| 1 | Compound message container | Base callback 0x10031b70 extracts the block through 0x10033df0, iterates inner headers via 0x10033ec0/0x10033ee0, and recursively dispatches at 0x10031c13. Implemented with nesting/count limits. |
| 313 | Gaming-profile edit result | Lobby callback 0x10003354 compares status with 1 at 0x10003371 for success, 2 for nickname-used, 120 for obscenity rejection, and other nonzero values for generic failure. Sent after 109. |
| 315 | Membership-profile edit result | Lobby callback 0x10003cee reads status and string, with status-1 success at 0x10003d3d. Failure text explicitly says Change Membership Profile Failed. Sent after 622. |
| 632 | Public user information request | Export TellMeUserPublicInfo 0x10012ec9 calls constructor 0x1000c15a at 0x10012f6e; that constructor selects 632 at 0x1000c1a6. |
| 316 | Public user information response | Callback 0x10003f40 fills SDK tPublicInfo, then invokes/posts its registered callback at 0x1000407c/0x1000409f. Implemented for existing local accounts. |

313/315 success is **1**, not a status copied from login. The server sends
negative generic failures for invalid/unsavable edits without disconnecting the
caller. It does not implement nickname/account renaming or the original obscenity
filter. Status zero in 313 is a quiet no-op, not its success-refresh branch.

### Public fields and privacy

`in-game-code/SNTYPES.H` defines tPublicInfo as four strings, two short LOGICAL
values and six SN_CODE bytes. Receive-handler offsets match that structure,
including USER_NAME_LEN=25, CITY_LEN=50, STATE_LEN=4 and COUNTRY_LEN=30. The
new response leaves room for each terminating NUL.

The local-user shortcut in TellMeUserPublicInfo reads cached city/state/country
at 0x1007317c/0x10073180/0x10073184. Linking 314 through member-object setter
0x1001d7f6 establishes membership strings 5, 6, 7 as those fields. The two
public shorts are membership shorts 0/1 (HasCD, HasMike); public byte fields
correspond to membership bytes 0–5 (AgeGroup, GenderGroup, ModemSpeedGroup,
MonitorGroup, RamCodeGroup, ProcessorGroup).

The server excludes real name, private address, password and email from 316.
It passes group codes through without guessing their numerical categories.
No original not-found result was established, so nonexistent users receive no
invented error message.

## Roles identified but not fully implemented

| ID | Finding | Evidence / remaining gap |
|---|---|---|
| 625 | Client-to-custom-server binary data | SendCustomServerMessage 0x100129f1 calls 0x100061d6 at 0x10012a0d; 625 selected at 0x100061f8. Backend application logic is not in the generic client DLL. |
| 505 | Custom-server binary response | Callback 0x10005563 reads blob length/data and posts through 0x1006d30c/0x1006d310. InitializeCustomServer sets exactly these globals at 0x100129e3/0x100129e8; FreeCustomServerMessage frees the buffer. No generic echo backend is assumed. |
| 633 | Peer activation notification | ActivateNewPlayerX 0x10011f7f finds a peer slot, marks +0x28 active, obtains its string and calls 0x10005f97 at 0x10012041; constructor emits 633. Complete server coordination sequence remains unestablished. |
| 618 | Room-data request path | Constructor 0x1000af71 sends an integer; caller 0x10023ba7 has an explicit retrieving Room data error. 312 calls the adjacent completion/reset helper 0x10023b85. Exact list contents and sequence are partial. |
| 501 | Transport status display | Callback 0x100051e1 displays Connecting to game server ...; original emission timing is not proven. |
| 502 | Transport result consumer | Callback 0x10005476 reads one integer and returns 1 without another operation in that body. |
| 305, 307, 413 | Field-consuming handlers | Non-default callbacks read fields and return without an operation in the inspected bodies. Sending them is not established to change state. |
| 312 | Room-data completion/reset | Callback 0x10003220 reads two integers and passes the second to 0x10023b85, which clears room-data state when nonzero. |
| 415, 416, 420 | Populated room event handlers | 0x100107bf, 0x100112e5 and 0x100114fe manipulate game/queued-event state. Triggers, identity roles and full event sequence need further tracing. |

### 627 permission values recovered from the SDK

SetGameState(Parameter, Value) at 0x10013293 calls 0x10018083, which emits 627
with game name and both integers. SNTYPES.H names parameters JOIN_PERMISSION=1,
INVITE_PERMISSION=2, WATCH_PERMISSION=3, JOIN_IN_PROGRESS=4.

| Parameter | Documented values |
|---|---|
| Join (1) | 1 nobody; 2 no approval; 3 any player approval; 4 captain only; 5 owner only; 6 majority and 7 unanimous marked not yet implemented in the SDK. |
| Invite (2) | 3 any player may invite; 1 owner-only, 2 captain-only and 4 room-wide marked not yet implemented. |
| Watch (3) | Marked not yet implemented. |
| Join in progress (4) | Constant exists; API comment does not define value semantics. |

The server now enforces join modes 1/2/5. Its existing owner policy remains the
first remaining member; historical captain/owner succession is unestablished.
Other values are recorded without changing the active policy. Real-game
permission changes and late-join activation remain unverified.

## Complete dispatch audit

The tool resolves IDs through the actual switch/jump tables at 0x10031040,
then reads virtual callback slots. Registry order is insufficient: 505 uses
+0x110 while 506 uses +0x104. It inspects documented lobby/room/transport tables
and other discovered matching tables. Message 1 has substantive base-handler
logic and must not be called ignored merely because subclasses do not override it.

The original 50-entry unresolved set has 13 IDs with overridden callbacks in
the inspected tables. The rest share base callbacks there. Four of the original
set have directly linked fixed-ID sender candidates: 618, 625, 632, 633.
Tracing resolved five entries into implemented behavior (1, 313, 315, 316, 632).
The [implementation audit](message-coverage.md) now retains 45 unsupported entries;
some have identified purposes but incomplete server behavior.

No direct fixed-ID sender for 621 was found by this scan; inspected receive
tables use a default callback. Its menu resource does not establish an active
password-change flow. Dynamic senders, other components and other builds remain
possible; credential mutation stays disabled until fields/replies are established.
Default handlers and absent direct senders do not prove universal historical non-use.

## Evidence and verification

The local `tools/research.py` uses pefile/Capstone libraries in
`third_party/research/` and the original DLL in `references/binaries/`.
Both `research.py` scripts, the analysis libraries and saved evidence are ignored
by Git and must be restored locally if absent. See [Git exclusions](version-control.md).
From the project root, using Python 3.10 or later on Windows x64:

```powershell
python tools/research.py exports
python tools/research.py constructors
python tools/research.py tables
python tools/research.py xref 0x100061d6
python tools/research.py range 0x10003cee 0x10003f40
```

Local machine outputs under the ignored `docs/research/` directory:
`dispatch-tables.json` and `message-constructors.json`.
Instruction evidence: `new-handler-evidence.txt`, `membership-object-setter.txt`,
`membership-and-state-setters.txt`, `base-handler-evidence.txt` and `block-parser-evidence.txt`.
Candidate instruction windows retain range-start/data-decoding caveats.

With the local research files present, CTest checks the callback map, 505/506
ordering and export-linked senders. Network
tests cover compound lengths/nesting, profile result IDs and success/failure,
public-info projection/privacy, and closed/open join permission. They execute
the new server, not the original DLL/client. No actual game or updater installation
compatibility has been established by these tests.
