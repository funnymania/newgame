// Asks for memory appropriately circumstantially.

struct Transfrom {
    V3 pos;
    V3 rot;
    V3 scale;
}

struct V3 {
    u64 x;
    u64 y;
    u64 z;
}

struct Ray {
    V3 begin;
    V3 end;
}

struct Sausage {
    u64 radius;
    u64 length;
    Transform tra;
}

struct MainThing {
    char* name;
    Sausage* targets;
    u32 *address; // Pointer to position of model in memory.
}

// note: For the game engine itself, as well as the game.
static MainThing* loaded_things;
static MainThing* loaded_things_table;

// returns a handle to place in memory.
u32* FillModelToMemory(char *model_name) {
    // Find model.    
    // Get size of model.
    // Allocate that space ONTO list of loaded_things_table.
    // Store reference to that space in loaded_ptrs.
}

// Raycasting to list of MainThing will perform calculation to determine whether the thing is being hit.
// Returns ptrs to hit MainThings.
vector<MainThing*> ThingsInRaycast(Ray ray, MainThing* loaded_things, u32 length) {
    vector<MainThing*> result;
    MainThing ptr = loaded_things;
    u32 i = 0;
    for(; i < length; i += 1) {
        result.push_back(ptr); 
        ptr += 1; 
    }

    return(result);
}

