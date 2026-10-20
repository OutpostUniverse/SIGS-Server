# Message coverage audit

This audit enumerates all 118 entries in the supplied SIERRANW 3.0.3.0 registry.
It does **not** claim that every entry has a recovered server implementation.
Generated from `sigs-server --message-coverage` after the service additions.

- **handled:** a receive path exists in its supported service context. This can
  include documented emulator policy, experimental adapters, or bounded storage
  of opaque state; it is not a claim of recovered historical semantics.
- **outbound:** the server emits this message for a supported trigger/configuration.
  Server replies do not become client requests merely because they share a registry.
- **codec-only:** encoding/decoding is available, but its original server trigger
  has not been implemented. Specifically, 101's connectivity probe is unrecovered.
- **unresolved:** layout known; no semantic handler or guessed reply. Receiving it
  records only ID, status, connection and byte count once per connection. That
  diagnostic is **not** counted as implementing the message.

Current counts: **34 handled**, **38 outbound**, **1 codec-only**, **45 unresolved**.

## Follow-up investigation

The [DLL/SDK investigation](message-investigation.md) recovered five additional
implemented entries: compound container 1, gaming-profile result 313,
membership-edit result 315, and public-info request/response 632/316. It also
identified the custom binary channel 625/505, activation sender 633, parts of
room-data flow 618/312, and documented meanings for 627 permission parameters.

45 entries still have no complete semantic server implementation. Some now have
an identified purpose but missing application logic, trigger or coordination
sequence. The `unresolved` status means that implementation remains incomplete;
it does not mean every purpose is unknown. These messages are not counted as
implemented merely because diagnostics can receive their IDs.

621 remains without established string roles/reply. The audit found no direct
fixed-ID sender through the inspected constructor pattern, and its inspected
receive callbacks are defaults. That is not proof of historical non-use.
Original server code, other components or action-linked captures may still be
needed for the remaining gaps. The original client has not been run against
this server; automated tests do not establish complete game compatibility.

## Complete registry

Registry labels are retained as provenance, including outdated `Unknown` labels;
the status column and service documentation describe the new implementation.

| ID | Status | Recovered layout | Registry label |
|---|---|---|---|
| 1 | handled | `%b` | Unknown1 |
| 2 | unresolved | `%s %b` | Unknown2 |
| 101 | codec-only | `%s %i` | RoomServerConnectivityResult |
| 102 | handled | `%s %s %i` | GameConnectionRequest |
| 103 | outbound | `%s %s %i %i` | GameTransportConfiguration |
| 104 | handled | `%s` | ExitNotification104 |
| 105 | unresolved | `%s` | Unknown105 |
| 106 | handled | `%s` | GameControlDisconnect |
| 107 | handled | `(empty)` | PingResponse |
| 108 | unresolved | `%s %i %i` | Unknown108 |
| 109 | handled | `%s %s %c %s %s` | UpdateGamingProfile |
| 110 | outbound | `%s %i` | UDPTestEndpoint |
| 111 | handled | `%s %s %c %s %s %s %i` | RoomPlayerRecord |
| 201 | outbound | `%s` | RoomMemberNotification |
| 202 | outbound | `%s` | Unknown202 |
| 203 | unresolved | `%i` | Unknown203 |
| 204 | unresolved | `%i` | Unknown204 |
| 205 | unresolved | `%s %s %i` | Unknown205 |
| 206 | unresolved | `%s` | Unknown206 |
| 207 | outbound | `(empty)` | Ping |
| 208 | unresolved | `%s %s %i` | Unknown208 |
| 209 | unresolved | `%i %i %s %s %s %s %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i` | Unknown209 |
| 301 | outbound | `%f` | DLLVersionMismatch |
| 302 | outbound | `%i %s %s` | SignupResultCandidate |
| 303 | outbound | `%i` | RoomJoinStatus |
| 304 | outbound | `%i` | RoomCreationStatusResponse |
| 305 | unresolved | `%i` | Unknown |
| 306 | outbound | `%s %s %i %i %i %i %c %s %s %s %i` | AddRoom |
| 307 | unresolved | `%s %s %i` | Unknown307 |
| 308 | outbound | `%i` | FindOpponentResult |
| 309 | outbound | `%s` | RemoveRoom |
| 310 | outbound | `%i %i` | UpdateRoomPopulation |
| 311 | outbound | `%i %s` | LoginResult_candidate |
| 312 | unresolved | `%i %i` | Unknown312 |
| 313 | outbound | `%i` | Unknown313 |
| 314 | outbound | `%s %s %s %s %s %s %s %s %s %s %s %l %l %i %c %c %c %c %c %c` | MemberProfile |
| 315 | outbound | `%i %s` | Unknown315 |
| 316 | outbound | `%s %s %s %s %l %l %c %c %c %c %c %c` | Unknown316 |
| 317 | unresolved | `(empty)` | Unknown317 |
| 401 | outbound | `%i %i %i %i %i %i %i %i %i %i %i %i %i %i` | RoomInitialization |
| 402 | outbound | `%i %i` | RoomListCompletion |
| 403 | outbound | `%s %i %i %i %i %s %i` | GameListing |
| 404 | outbound | `%i %s %i` | GameCreationStatus |
| 405 | outbound | `%s` | GameStartedNotification |
| 406 | outbound | `%s` | RemoveGame |
| 407 | outbound | `%s %i %i %s` | GameInvitation |
| 408 | outbound | `%i %s` | InvitationResult |
| 409 | outbound | `%s %i %i %s` | GameJoinPermissionRequest |
| 410 | outbound | `%i` | GameJoinApproval |
| 411 | outbound | `%s %i %i %s` | GameMemberJoined |
| 412 | outbound | `%s %i %i %s` | GameMemberLeft |
| 413 | unresolved | `%i` | Unknown413 |
| 414 | unresolved | `%i %s %b` | Unknown414 |
| 415 | unresolved | `%s %i %i` | Unknown415 |
| 416 | unresolved | `%s %s` | Unknown416 |
| 417 | unresolved | `%i` | Unknown417 |
| 418 | unresolved | `%s %i %s` | Unknown418 |
| 419 | unresolved | `%s %i %i %s` | Unknown419 |
| 420 | unresolved | `%s %i %i %s` | Unknown420 |
| 421 | unresolved | `%s %s %s %i` | Unknown421 |
| 422 | unresolved | `%s %s %s %i` | Unknown422 |
| 423 | unresolved | `%s %i` | Unknown423 |
| 424 | unresolved | `%i %i` | Unknown424 |
| 501 | unresolved | `(empty)` | Unknown501 |
| 502 | unresolved | `%i` | Unknown502 |
| 503 | unresolved | `(empty)` | Unknown503 |
| 504 | unresolved | `(empty)` | Unknown504 |
| 505 | unresolved | `%b` | Unknown505 |
| 506 | unresolved | `(empty)` | Unknown506 |
| 601 | handled | `%s %s` | Login |
| 602 | handled | `%i` | JoinRoom |
| 603 | handled | `%s %s %s %i %c %s` | CreateRoom |
| 604 | unresolved | `(empty)` | Unknown |
| 605 | handled | `%s %i %i %i %i %i %i` | CreateGame |
| 606 | handled | `%s %i %i %s` | InvitePlayer |
| 607 | handled | `%s %i` | InviteReply |
| 608 | handled | `%s %s %i %i` | RequestGameJoin |
| 609 | handled | `%s %i` | GameJoinDecision |
| 610 | unresolved | `%s %i %s` | Unknown |
| 611 | unresolved | `%s %i` | Unknown |
| 612 | handled | `%s` | FindOpponent |
| 613 | unresolved | `%l` | Unknown |
| 614 | unresolved | `(empty)` | Unknown |
| 615 | handled | `(empty)` | LeaveGameRequest |
| 616 | handled | `%s %s` | RequestSIGSVersionCheck |
| 617 | handled | `(empty)` | RequestUpdateDownload |
| 618 | unresolved | `%i` | Unknown618 |
| 619 | handled | `(empty)` | StartGameRequest |
| 620 | handled | `%s %s %s %s %s %s %s %s %s %s %s %s %l %l %i %c %c %c %c %c %c` | SignupRequest |
| 621 | unresolved | `%s %s` | PasswordChangeRequest_candidate |
| 622 | handled | `%s %s %s %s %s %s %s %s %s %s %s %s %s %s %l %l %i %c %c %c %c %c %c` | UpdateMemberProfile |
| 623 | unresolved | `%s` | Unknown623 |
| 624 | unresolved | `%s %s` | Unknown624 |
| 625 | unresolved | `%b` | Unknown625 |
| 626 | handled | `%s %s` | RequestGameVersionCheck |
| 627 | handled | `%s %i %i` | SetGameState |
| 628 | unresolved | `%s %i` | Unknown628 |
| 629 | unresolved | `%s %i %i` | Unknown629 |
| 630 | unresolved | `%s %i %i %s` | Unknown630 |
| 631 | handled | `%s %s` | UserComplaint |
| 632 | handled | `%s` | Unknown632 |
| 633 | unresolved | `%s` | Unknown633 |
| 634 | handled | `%s` | UDPTestRequest |
| 701 | handled | `%s %s` | PublicRoomChat |
| 702 | handled | `%s %s %s` | PrivateRoomChat |
| 703 | handled | `%b` | TCPBroadcastData |
| 704 | handled | `%i %i %b` | TCPPointData |
| 801 | handled | `%s` | Unknown801 |
| 802 | handled | `%s %s` | Unknown802 |
| 803 | outbound | `%i %s %s` | Unknown803 |
| 804 | handled | `%s` | Unknown804 |
| 901 | outbound | `%s %i` | ConnectionSuccessful |
| 902 | outbound | `(empty)` | Unknown |
| 903 | outbound | `%s %i` | Unknown |
| 904 | outbound | `%i` | Unknown |
| 905 | outbound | `%b` | Unknown |
| 906 | outbound | `(empty)` | Unknown |
| 907 | outbound | `%i %i %l` | GameRequiresUpdate |
