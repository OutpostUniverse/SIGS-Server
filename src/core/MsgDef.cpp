// Reconstructed from supplied DLL registries. No original source claimed.
#include "MsgDef.h"

namespace sigs::core
{
    static const MessageDefinition kSIERRANW[] = {
        {1, "%b", "Unknown1"},                              // register 0x10032425; format 0x1006ffcc
        {2, "%s %b", "Unknown2"},                           // register 0x10032450; format 0x10070394
        {101, "%s %i", "RoomServerConnectivityResult"},     // register 0x1003247b; format 0x1007038c
        {102, "%s %s %i", "GameConnectionRequest"},         // register 0x100324a6; format 0x10070380
        {103, "%s %s %i %i", "GameTransportConfiguration"}, // register 0x100324d1; format 0x10070374
        {104, "%s", "ExitNotification104"},                 // register 0x100324fc; format 0x1006ffd0
        {105, "%s", "Unknown105"},                          // register 0x10032527; format 0x1006ffd0
        {106, "%s", "GameControlDisconnect"},               // register 0x10032552; format 0x1006ffd0
        {107, "", "PingResponse"},                          // register 0x1003257d; format 0x1006c154
        {108, "%s %i %i", "Unknown108"},                    // register 0x100325a8; format 0x10070368
        {109, "%s %s %c %s %s", "UpdateGamingProfile"},     // register 0x100325d3; format 0x10070358
        {110, "%s %i", "UDPTestEndpoint"},                  // register 0x100325fe; format 0x1007038c
        {111, "%s %s %c %s %s %s %i", "RoomPlayerRecord"},  // register 0x10032629; format 0x10070340
        {201, "%s", "RoomMemberNotification"},              // register 0x10032657; format 0x1006ffd0
        {202, "%s", "Unknown202"},                          // register 0x10032685; format 0x1006ffd0
        {203, "%i", "Unknown203"},                          // register 0x100326b3; format 0x1006ffe4
        {204, "%i", "Unknown204"},                          // register 0x100326e1; format 0x1006ffe4
        {205, "%s %s %i", "Unknown205"},                    // register 0x1003270f; format 0x10070380
        {206, "%s", "Unknown206"},                          // register 0x1003273d; format 0x1006ffd0
        {207, "", "Ping"},                                  // register 0x1003276b; format 0x1006c154
        {208, "%s %s %i", "Unknown208"},                    // register 0x10032799; format 0x10070380
        {209, "%i %i %s %s %s %s %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
         "Unknown209"},                                       // register 0x100327c7; format 0x10070300
        {301, "%f", "DLLVersionMismatch"},                    // register 0x100327f5; format 0x1006ffe0
        {302, "%i %s %s", "SignupResultCandidate"},           // register 0x10032823; format 0x100702f4
        {303, "%i", "RoomJoinStatus"},                        // register 0x10032851; format 0x1006ffe4
        {304, "%i", "RoomCreationStatusResponse"},            // register 0x1003287f; format 0x1006ffe4
        {305, "%i", "Unknown"},                               // register 0x100328ad; format 0x1006ffe4
        {306, "%s %s %i %i %i %i %c %s %s %s %i", "AddRoom"}, // register 0x100328db; format 0x100702d0
        {307, "%s %s %i", "Unknown307"},                      // register 0x10032909; format 0x10070380
        {308, "%i", "FindOpponentResult"},                    // register 0x10032937; format 0x1006ffe4
        {309, "%s", "RemoveRoom"},                            // register 0x10032965; format 0x1006ffd0
        {310, "%i %i", "UpdateRoomPopulation"},               // register 0x10032993; format 0x100702c8
        {311, "%i %s", "LoginResult_candidate"},              // register 0x100329c1; format 0x100702c0
        {312, "%i %i", "Unknown312"},                         // register 0x100329ef; format 0x100702c8
        {313, "%i", "Unknown313"},                            // register 0x10032a1d; format 0x1006ffe4
        {314, "%s %s %s %s %s %s %s %s %s %s %s %l %l %i %c %c %c %c %c %c",
         "MemberProfile"},                                          // register 0x10032a4b; format 0x10070284
        {315, "%i %s", "Unknown315"},                               // register 0x10032a79; format 0x100702c0
        {316, "%s %s %s %s %l %l %c %c %c %c %c %c", "Unknown316"}, // register 0x10032aa7; format 0x10070260
        {317, "", "Unknown317"},                                    // register 0x10032ad5; format 0x1006c154
        {401, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i",
         "RoomInitialization"},                            // register 0x10032b03; format 0x10070234
        {402, "%i %i", "RoomListCompletion"},              // register 0x10032b31; format 0x100702c8
        {403, "%s %i %i %i %i %s %i", "GameListing"},      // register 0x10032b5f; format 0x1007021c
        {404, "%i %s %i", "GameCreationStatus"},           // register 0x10032b8d; format 0x10070210
        {405, "%s", "GameStartedNotification"},            // register 0x10032bbb; format 0x1006ffd0
        {406, "%s", "RemoveGame"},                         // register 0x10032be9; format 0x1006ffd0
        {407, "%s %i %i %s", "GameInvitation"},            // register 0x10032c17; format 0x10070204
        {408, "%i %s", "InvitationResult"},                // register 0x10032c45; format 0x100702c0
        {409, "%s %i %i %s", "GameJoinPermissionRequest"}, // register 0x10032c73; format 0x10070204
        {410, "%i", "GameJoinApproval"},                   // register 0x10032ca1; format 0x1006ffe4
        {411, "%s %i %i %s", "GameMemberJoined"},          // register 0x10032ccf; format 0x10070204
        {412, "%s %i %i %s", "GameMemberLeft"},            // register 0x10032cfd; format 0x10070204
        {413, "%i", "Unknown413"},                         // register 0x10032d2b; format 0x1006ffe4
        {414, "%i %s %b", "Unknown414"},                   // register 0x10032d59; format 0x100701f8
        {415, "%s %i %i", "Unknown415"},                   // register 0x10032d87; format 0x10070368
        {416, "%s %s", "Unknown416"},                      // register 0x10032db5; format 0x100701f0
        {417, "%i", "Unknown417"},                         // register 0x10032de3; format 0x1006ffe4
        {418, "%s %i %s", "Unknown418"},                   // register 0x10032e11; format 0x100701e4
        {419, "%s %i %i %s", "Unknown419"},                // register 0x10032e3f; format 0x10070204
        {420, "%s %i %i %s", "Unknown420"},                // register 0x10032e6d; format 0x10070204
        {421, "%s %s %s %i", "Unknown421"},                // register 0x10032e9b; format 0x100701d8
        {422, "%s %s %s %i", "Unknown422"},                // register 0x10032ec9; format 0x100701d8
        {423, "%s %i", "Unknown423"},                      // register 0x10032ef7; format 0x1007038c
        {424, "%i %i", "Unknown424"},                      // register 0x10032f25; format 0x100702c8
        {501, "", "Unknown501"},                           // register 0x10032f53; format 0x1006c154
        {502, "%i", "Unknown502"},                         // register 0x10032f81; format 0x1006ffe4
        {503, "", "Unknown503"},                           // register 0x10032faf; format 0x1006c154
        {504, "", "Unknown504"},                           // register 0x10032fdd; format 0x1006c154
        {505, "%b", "Unknown505"},                         // register 0x1003300b; format 0x1006ffcc
        {506, "", "Unknown506"},                           // register 0x10033039; format 0x1006c154
        {601, "%s %s", "Login"},                           // register 0x10033067; format 0x100701f0
        {602, "%i", "JoinRoom"},                           // register 0x10033095; format 0x1006ffe4
        {603, "%s %s %s %i %c %s", "CreateRoom"},          // register 0x100330c3; format 0x100701c4
        {604, "", "Unknown"},                              // register 0x100330f1; format 0x1006c154
        {605, "%s %i %i %i %i %i %i", "CreateGame"},       // register 0x1003311f; format 0x100701ac
        {606, "%s %i %i %s", "InvitePlayer"},              // register 0x1003314d; format 0x10070204
        {607, "%s %i", "InviteReply"},                     // register 0x1003317b; format 0x1007038c
        {608, "%s %s %i %i", "RequestGameJoin"},           // register 0x100331a9; format 0x10070374
        {609, "%s %i", "GameJoinDecision"},                // register 0x100331d7; format 0x1007038c
        {610, "%s %i %s", "Unknown"},                      // register 0x10033205; format 0x100701e4
        {611, "%s %i", "Unknown"},                         // register 0x10033233; format 0x1007038c
        {612, "%s", "FindOpponent"},                       // register 0x10033261; format 0x1006ffd0
        {613, "%l", "Unknown"},                            // register 0x1003328f; format 0x1006ffd8
        {614, "", "Unknown"},                              // register 0x100332bd; format 0x1006c154
        {615, "", "LeaveGameRequest"},                     // register 0x100332eb; format 0x1006c154
        {616, "%s %s", "RequestSIGSVersionCheck"},         // register 0x10033319; format 0x100701f0
        {617, "", "RequestUpdateDownload"},                // register 0x10033347; format 0x1006c154
        {618, "%i", "Unknown618"},                         // register 0x10033375; format 0x1006ffe4
        {619, "", "StartGameRequest"},                     // register 0x100333a3; format 0x1006c154
        {620, "%s %s %s %s %s %s %s %s %s %s %s %s %l %l %i %c %c %c %c %c %c",
         "SignupRequest"},                                 // register 0x100333d1; format 0x1007016c
        {621, "%s %s", "PasswordChangeRequest_candidate"}, // register 0x100333ff; format 0x100701f0
        {622, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %l %l %i %c %c %c %c %c %c",
         "UpdateMemberProfile"},                   // register 0x1003342d; format 0x10070124
        {623, "%s", "Unknown623"},                 // register 0x1003345b; format 0x1006ffd0
        {624, "%s %s", "Unknown624"},              // register 0x10033489; format 0x100701f0
        {625, "%b", "Unknown625"},                 // register 0x100334b7; format 0x1006ffcc
        {626, "%s %s", "RequestGameVersionCheck"}, // register 0x100334e5; format 0x100701f0
        {627, "%s %i %i", "SetGameState"},         // register 0x10033513; format 0x10070368
        {628, "%s %i", "Unknown628"},              // register 0x10033541; format 0x1007038c
        {629, "%s %i %i", "Unknown629"},           // register 0x1003356f; format 0x10070368
        {630, "%s %i %i %s", "Unknown630"},        // register 0x1003359d; format 0x10070204
        {631, "%s %s", "UserComplaint"},           // register 0x100335cb; format 0x100701f0
        {632, "%s", "Unknown632"},                 // register 0x100335f9; format 0x1006ffd0
        {633, "%s", "Unknown633"},                 // register 0x10033627; format 0x1006ffd0
        {634, "%s", "UDPTestRequest"},             // register 0x10033655; format 0x1006ffd0
        {701, "%s %s", "PublicRoomChat"},          // register 0x10033683; format 0x100701f0
        {702, "%s %s %s", "PrivateRoomChat"},      // register 0x100336b1; format 0x10070118
        {703, "%b", "TCPBroadcastData"},           // register 0x100336df; format 0x1006ffcc
        {704, "%i %i %b", "TCPPointData"},         // register 0x1003370d; format 0x1007010c
        {801, "%s", "Unknown801"},                 // register 0x1003373b; format 0x1006ffd0
        {802, "%s %s", "Unknown802"},              // register 0x10033769; format 0x100701f0
        {803, "%i %s %s", "Unknown803"},           // register 0x10033797; format 0x100702f4
        {804, "%s", "Unknown804"},                 // register 0x100337c5; format 0x1006ffd0
        {901, "%s %i", "ConnectionSuccessful"},    // register 0x100337f3; format 0x1007038c
        {902, "", "Unknown"},                      // register 0x10033821; format 0x1006c154
        {903, "%s %i", "Unknown"},                 // register 0x1003384f; format 0x1007038c
        {904, "%i", "Unknown"},                    // register 0x1003387d; format 0x1006ffe4
        {905, "%b", "Unknown"},                    // register 0x100338ab; format 0x1006ffcc
        {906, "", "Unknown"},                      // register 0x100338d9; format 0x1006c154
        {907, "%i %i %l", "GameRequiresUpdate"},   // register 0x10033907; format 0x10070100
    };
    static const MessageDefinition kSNWVALID[] = {
        {1, "%b", "Unknown1"},                              // register 0x10008c55; format 0x10028478
        {2, "%s %b", "Unknown2"},                           // register 0x10008c80; format 0x10028844
        {101, "%s %i", "RoomServerConnectivityResult"},     // register 0x10008cab; format 0x1002883c
        {102, "%s %s %i", "GameConnectionRequest"},         // register 0x10008cd6; format 0x10028830
        {103, "%s %s %i %i", "GameTransportConfiguration"}, // register 0x10008d01; format 0x10028824
        {104, "%s", "ExitNotification104"},                 // register 0x10008d2c; format 0x1002847c
        {105, "%s", "Unknown105"},                          // register 0x10008d57; format 0x1002847c
        {106, "%s", "GameControlDisconnect"},               // register 0x10008d82; format 0x1002847c
        {107, "", "PingResponse"},                          // register 0x10008dad; format 0x1002708c
        {108, "%s %i %i", "Unknown108"},                    // register 0x10008dd8; format 0x10028818
        {109, "%s %s %c %s %s", "UpdateGamingProfile"},     // register 0x10008e03; format 0x10028808
        {110, "%s %i", "UDPTestEndpoint"},                  // register 0x10008e2e; format 0x1002883c
        {111, "%s %s %c %s %s %s %i", "RoomPlayerRecord"},  // register 0x10008e59; format 0x100287f0
        {201, "%s", "RoomMemberNotification"},              // register 0x10008e87; format 0x1002847c
        {202, "%s", "Unknown202"},                          // register 0x10008eb5; format 0x1002847c
        {203, "%i", "Unknown203"},                          // register 0x10008ee3; format 0x10028494
        {204, "%i", "Unknown204"},                          // register 0x10008f11; format 0x10028494
        {205, "%s %s %i", "Unknown205"},                    // register 0x10008f3f; format 0x10028830
        {206, "%s", "Unknown206"},                          // register 0x10008f6d; format 0x1002847c
        {207, "", "Ping"},                                  // register 0x10008f9b; format 0x1002708c
        {208, "%s %s %i", "Unknown208"},                    // register 0x10008fc9; format 0x10028830
        {209, "%i %i %s %s %s %s %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
         "Unknown209"},                                       // register 0x10008ff7; format 0x100287b0
        {301, "%f", "DLLVersionMismatch"},                    // register 0x10009025; format 0x10028490
        {302, "%i %s %s", "SignupResultCandidate"},           // register 0x10009053; format 0x100287a4
        {303, "%i", "RoomJoinStatus"},                        // register 0x10009081; format 0x10028494
        {304, "%i", "RoomCreationStatusResponse"},            // register 0x100090af; format 0x10028494
        {305, "%i", "Unknown"},                               // register 0x100090dd; format 0x10028494
        {306, "%s %s %i %i %i %i %c %s %s %s %i", "AddRoom"}, // register 0x1000910b; format 0x10028780
        {307, "%s %s %i", "Unknown307"},                      // register 0x10009139; format 0x10028830
        {308, "%i", "FindOpponentResult"},                    // register 0x10009167; format 0x10028494
        {309, "%s", "RemoveRoom"},                            // register 0x10009195; format 0x1002847c
        {310, "%i %i", "UpdateRoomPopulation"},               // register 0x100091c3; format 0x10028778
        {311, "%i %s", "LoginResult_candidate"},              // register 0x100091f1; format 0x10028770
        {312, "%i %i", "Unknown312"},                         // register 0x1000921f; format 0x10028778
        {313, "%i", "Unknown313"},                            // register 0x1000924d; format 0x10028494
        {314, "%s %s %s %s %s %s %s %s %s %s %s %l %l %i %c %c %c %c %c %c",
         "MemberProfile"},                                          // register 0x1000927b; format 0x10028734
        {315, "%i %s", "Unknown315"},                               // register 0x100092a9; format 0x10028770
        {316, "%s %s %s %s %l %l %c %c %c %c %c %c", "Unknown316"}, // register 0x100092d7; format 0x10028710
        {317, "", "Unknown317"},                                    // register 0x10009305; format 0x1002708c
        {401, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i",
         "RoomInitialization"},                            // register 0x10009333; format 0x100286e4
        {402, "%i %i", "RoomListCompletion"},              // register 0x10009361; format 0x10028778
        {403, "%s %i %i %i %i %s %i", "GameListing"},      // register 0x1000938f; format 0x100286cc
        {404, "%i %s %i", "GameCreationStatus"},           // register 0x100093bd; format 0x100286c0
        {405, "%s", "GameStartedNotification"},            // register 0x100093eb; format 0x1002847c
        {406, "%s", "RemoveGame"},                         // register 0x10009419; format 0x1002847c
        {407, "%s %i %i %s", "GameInvitation"},            // register 0x10009447; format 0x100286b4
        {408, "%i %s", "InvitationResult"},                // register 0x10009475; format 0x10028770
        {409, "%s %i %i %s", "GameJoinPermissionRequest"}, // register 0x100094a3; format 0x100286b4
        {410, "%i", "GameJoinApproval"},                   // register 0x100094d1; format 0x10028494
        {411, "%s %i %i %s", "GameMemberJoined"},          // register 0x100094ff; format 0x100286b4
        {412, "%s %i %i %s", "GameMemberLeft"},            // register 0x1000952d; format 0x100286b4
        {413, "%i", "Unknown413"},                         // register 0x1000955b; format 0x10028494
        {414, "%i %s %b", "Unknown414"},                   // register 0x10009589; format 0x100286a8
        {415, "%s %i %i", "Unknown415"},                   // register 0x100095b7; format 0x10028818
        {416, "%s %s", "Unknown416"},                      // register 0x100095e5; format 0x100286a0
        {417, "%i", "Unknown417"},                         // register 0x10009613; format 0x10028494
        {418, "%s %i %s", "Unknown418"},                   // register 0x10009641; format 0x10028694
        {419, "%s %i %i %s", "Unknown419"},                // register 0x1000966f; format 0x100286b4
        {420, "%s %i %i %s", "Unknown420"},                // register 0x1000969d; format 0x100286b4
        {421, "%s %s %s %i", "Unknown421"},                // register 0x100096cb; format 0x10028688
        {422, "%s %s %s %i", "Unknown422"},                // register 0x100096f9; format 0x10028688
        {423, "%s %i", "Unknown423"},                      // register 0x10009727; format 0x1002883c
        {424, "%i %i", "Unknown424"},                      // register 0x10009755; format 0x10028778
        {501, "", "Unknown501"},                           // register 0x10009783; format 0x1002708c
        {502, "%i", "Unknown502"},                         // register 0x100097b1; format 0x10028494
        {503, "", "Unknown503"},                           // register 0x100097df; format 0x1002708c
        {504, "", "Unknown504"},                           // register 0x1000980d; format 0x1002708c
        {505, "%b", "Unknown505"},                         // register 0x1000983b; format 0x10028478
        {506, "", "Unknown506"},                           // register 0x10009869; format 0x1002708c
        {601, "%s %s", "Login"},                           // register 0x10009897; format 0x100286a0
        {602, "%i", "JoinRoom"},                           // register 0x100098c5; format 0x10028494
        {603, "%s %s %s %i %c %s", "CreateRoom"},          // register 0x100098f3; format 0x10028674
        {604, "", "Unknown"},                              // register 0x10009921; format 0x1002708c
        {605, "%s %i %i %i %i %i %i", "CreateGame"},       // register 0x1000994f; format 0x1002865c
        {606, "%s %i %i %s", "InvitePlayer"},              // register 0x1000997d; format 0x100286b4
        {607, "%s %i", "InviteReply"},                     // register 0x100099ab; format 0x1002883c
        {608, "%s %s %i %i", "RequestGameJoin"},           // register 0x100099d9; format 0x10028824
        {609, "%s %i", "GameJoinDecision"},                // register 0x10009a07; format 0x1002883c
        {610, "%s %i %s", "Unknown"},                      // register 0x10009a35; format 0x10028694
        {611, "%s %i", "Unknown"},                         // register 0x10009a63; format 0x1002883c
        {612, "%s", "FindOpponent"},                       // register 0x10009a91; format 0x1002847c
        {613, "%l", "Unknown"},                            // register 0x10009abf; format 0x10028488
        {614, "", "Unknown"},                              // register 0x10009aed; format 0x1002708c
        {615, "", "LeaveGameRequest"},                     // register 0x10009b1b; format 0x1002708c
        {616, "%s %s", "RequestSIGSVersionCheck"},         // register 0x10009b49; format 0x100286a0
        {617, "", "RequestUpdateDownload"},                // register 0x10009b77; format 0x1002708c
        {618, "%i", "Unknown618"},                         // register 0x10009ba5; format 0x10028494
        {619, "", "StartGameRequest"},                     // register 0x10009bd3; format 0x1002708c
        {620, "%s %s %s %s %s %s %s %s %s %s %s %s %l %l %i %c %c %c %c %c %c",
         "SignupRequest"},                                 // register 0x10009c01; format 0x1002861c
        {621, "%s %s", "PasswordChangeRequest_candidate"}, // register 0x10009c2f; format 0x100286a0
        {622, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %l %l %i %c %c %c %c %c %c",
         "UpdateMemberProfile"},                   // register 0x10009c5d; format 0x100285d4
        {623, "%s", "Unknown623"},                 // register 0x10009c8b; format 0x1002847c
        {624, "%s %s", "Unknown624"},              // register 0x10009cb9; format 0x100286a0
        {625, "%b", "Unknown625"},                 // register 0x10009ce7; format 0x10028478
        {626, "%s %s", "RequestGameVersionCheck"}, // register 0x10009d15; format 0x100286a0
        {627, "%s %i %i", "SetGameState"},         // register 0x10009d43; format 0x10028818
        {628, "%s %i", "Unknown628"},              // register 0x10009d71; format 0x1002883c
        {629, "%s %i %i", "Unknown629"},           // register 0x10009d9f; format 0x10028818
        {630, "%s %i %i %s", "Unknown630"},        // register 0x10009dcd; format 0x100286b4
        {631, "%s %s", "UserComplaint"},           // register 0x10009dfb; format 0x100286a0
        {632, "%s", "Unknown632"},                 // register 0x10009e29; format 0x1002847c
        {633, "%s", "Unknown633"},                 // register 0x10009e57; format 0x1002847c
        {634, "%s", "UDPTestRequest"},             // register 0x10009e85; format 0x1002847c
        {701, "%s %s", "PublicRoomChat"},          // register 0x10009eb3; format 0x100286a0
        {702, "%s %s %s", "PrivateRoomChat"},      // register 0x10009ee1; format 0x100285c8
        {703, "%b", "TCPBroadcastData"},           // register 0x10009f0f; format 0x10028478
        {704, "%i %i %b", "TCPPointData"},         // register 0x10009f3d; format 0x100285bc
        {801, "%s", "Unknown801"},                 // register 0x10009f6b; format 0x1002847c
        {802, "%s %s", "Unknown802"},              // register 0x10009f99; format 0x100286a0
        {803, "%i %s %s", "Unknown803"},           // register 0x10009fc7; format 0x100287a4
        {804, "%s", "Unknown804"},                 // register 0x10009ff5; format 0x1002847c
        {901, "%s %i", "ConnectionSuccessful"},    // register 0x1000a023; format 0x1002883c
        {902, "", "Unknown"},                      // register 0x1000a051; format 0x1002708c
        {903, "%s %i", "Unknown"},                 // register 0x1000a07f; format 0x1002883c
        {904, "%i", "Unknown"},                    // register 0x1000a0ad; format 0x10028494
        {905, "%b", "Unknown"},                    // register 0x1000a0db; format 0x10028478
        {906, "", "Unknown"},                      // register 0x1000a109; format 0x1002708c
        {907, "%i %i %l", "GameRequiresUpdate"},   // register 0x1000a137; format 0x100285b0
    };

    const MessageDefinition* GetMessageDefinitions(Registry registry, size_t* count)
    {
        const MessageDefinition* definitions = 0;
        size_t size = 0;
        switch (registry)
        {
        case SierraNW:
            definitions = kSIERRANW;
            size = sizeof(kSIERRANW) / sizeof(kSIERRANW[0]);
            break;
        case SNWValid:
            definitions = kSNWVALID;
            size = sizeof(kSNWVALID) / sizeof(kSNWVALID[0]);
            break;
        }
        if (count)
        {
            *count = size;
        }
        return definitions;
    }

    const MessageDefinition* FindMessageDefinition(Registry registry, unsigned int id)
    {
        size_t count = 0;
        const MessageDefinition* definitions = GetMessageDefinitions(registry, &count);
        for (size_t definitionIndex = 0; definitionIndex < count; ++definitionIndex)
        {
            if (definitions[definitionIndex].id == id)
            {
                return definitions + definitionIndex;
            }
        }
        return 0;
    }
} // namespace sigs::core
