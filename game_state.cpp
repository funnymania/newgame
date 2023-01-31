#include "game_state.h"
#include "interpolated_pattern.h"

struct Peer {
    u8 ip[4];
    u32 port;
    wchar_t name[36];
    wchar_t name_as_key[40];
    wchar_t tagline[100];
    bool online;
};

struct PeerLookupResult {
    Peer peer;
    bool success;
};

enum SpaceCode 
{
    GridView,
    InSpace,
};

struct Space {
    u64 id;
    SimpList<Peer> editors;
    SimpList<Peer> visitors; 
};

struct GameState 
{
    SpaceCode visiting_space; // '0' can be not in a space.
    v3i64 view_move;
    SimpList<Space> residencies;
    SimpList<Peer> peers;
};

struct SpaceGrid
{
    u64 seek;
    SimpList<Space> spaces;
};

auto LookUpFollowedBy(SimpList<Peer> followers, u8 ip[4], u32 port) -> PeerLookupResult
{
    for (u32 f = 0; f < followers.length; f += 1) {
        if (followers.array[f].ip[0] == ip[0] 
            && followers.array[f].ip[1] == ip[1] 
            && followers.array[f].ip[2] == ip[2] 
            && followers.array[f].ip[3] == ip[3] 
            && followers.array[f].port == port
        ) {
            return PeerLookupResult { followers.array[f], true };
        }
    }

    return PeerLookupResult { 0, false };
}

// auto LookUpFollowing(SimpList<Peer> following, u8 ip[4], u32 port) -> PeerLookupResult
// {
//     for (auto f : following.array) {
//         if (f.ip == ip && f.port == f.port) {
//             return PeerLookupResult { f, true };
//         }
//     }
// 
//     return PeerLookupResult { 0, false };
// }
