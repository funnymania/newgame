#include "game_state.h"
#include "interpolated_pattern.h"

// note: An entry in the peerlist.
// study: consider span/string for IP, and also a u8 array[3]
struct Peer {
    char ip_address[12];
    bool online;
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


