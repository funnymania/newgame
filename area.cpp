
// Contains Assets which are loaded, stored in Area files.
struct Area 
{
    // todo: grab memory and assign it, we need something like a growable array.
    Obj models[64]; 
};

static Area LoadArea(char* name, GameMemory* game_memory) 
{
    Area result = {};
    // read this file in.
    
    // Assuming this involves cube.obj in the following position.
    // todo: develop loading schemes.
    // Load models into memory. 
    Obj* cube = LoadOBJToMemory("cube.obj", game_memory);

    // move the cube.
    cube->tra.pos = { 0, 0, 2 };

    result.models[0] = *cube; // Copy.
    free(cube);

    return(result);
}

