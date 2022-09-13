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
        tmp += 1;
    }
}

struct LevelResults
{
    InterpolatedPattern path;
};

static LevelResults results;

void Setup(GameMemory* game_memory) 
{
    results = {};

    // path.
    List<v4i64> path = {};
    AddToList<v4i64>(&path, { 0,0,0,0 }, game_memory);
    AddToList<v4i64>(&path, { -300,0,400,59 }, game_memory);
    AddToList<v4i64>(&path, { -100,0,0,179 }, game_memory);
    AddToList<v4i64>(&path, {  400,0,300,299 }, game_memory);

    results.path = InterpolatedPattern::Create(path, game_memory);

    GameMemoryFree(path.array, 4, sizeof(v4i64)); 
}

LevelResults* ExecuteLevel() 
{
    return(&results); 
}

static Area LoadArea(int id, GameMemory* game_memory, PlatformServices services, DoubleInterpolatedPattern* rumbles) 
{
    Area result = {};

    // Load models into memory. 
    Obj* cube = services.load_obj("cube.obj", game_memory);

    // move the cube.
    cube->tra.pos = { 0, 0, 2 };

    // example of how to set rumble.
    // note: services.rumble_controller is expected to be called each frame.
    // f32 first_arr[3][2] = {{0, 0}, {20000, 29}, {0, 59}};
    // Sequence_f32 first = Sequence_f32::Create(first_arr, 3, game_memory);
    // Sequence_f32 second = Sequence_f32::Create(first_arr, 3, game_memory);
    // rumbles[0] = DoubleInterpolatedPattern::Create(first, second);
    // services.rumble_controller(0, rumbles);

    result.models[0] = *cube; // Copy.
    GameMemoryFree(cube, 1, sizeof(Obj));

    Setup(game_memory);

    return(result);
}
