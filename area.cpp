#include "platform_services.h"

// Contains Assets which are loaded, stored in Area files.
struct Area 
{
    // todo: grab memory and assign it, we need something like a growable array.
    Obj models[64]; 
};

static void SwitchArea(char* name, GameMemory* game_memory)
{
    // Check to see if this level is already in memory.
    //       if so switch..
    //       else LoadArea() and then switch
}


static Area LoadArea(char* name, GameMemory* game_memory, PlatformServices services) 
{
    Area result = {};

    // study: come to a decision on in-game loading of assets, THEN build the developer LevelSwapping around that.
    // Load area.
    //
    // these files will be read to once, and then written to often (or maybe just written to on some timer... )
    
    // Assuming this involves cube.obj in the following position.
    // todo: develop loading schemes.
    // Load models into memory. 
    Obj* cube = services.load_obj("cube.obj", game_memory);

    // move the cube.
    cube->tra.pos = { 0, 0, 2 };

    result.models[0] = *cube; // Copy.
    free(cube);

    return(result);
}
