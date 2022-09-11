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

void GameMemoryFree(void* obj_ptr, u32 obj_count, u32 obj_size) 
{
    u8* tmp = (u8*)obj_ptr;
    for (int counter = 0; counter < obj_size * obj_count; counter += 1) 
    {
        *tmp = 0;
    }
}


static Area LoadArea(int id, GameMemory* game_memory, PlatformServices services) 
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
    GameMemoryFree(cube, 1, sizeof(Obj));

    return(result);
}
