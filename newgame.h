#ifndef NEWGAME_H
#define NEWGAME_H

// study: How to do lookup table given manual memory management.
struct SoundAssetTable {
    AudioAsset value;
    SoundAssetTable* next;
};

static SoundAssetTable* Add(SoundAssetTable* table, AudioAsset asset, GameMemory* game_memory);
static AudioAsset* Get(SoundAssetTable* table, char* name);

// void LoadSound(char* file_name);
SoundPlayResult StartPlaying(char* name);

// INPUT.
struct Input
{
    v2_i64* lr;  // 2D input, X is left and right, Y is forward and back.
    v2_i64* z;   // 2D input, X is undefined, Y is up and down.
    u16 interact_mask;   // Example: if `2`, this means that we AND AngelInput's buttons with 2 to get the result.
};

// Buffer which we map real input to. This is so that the user can config what their buttons/inputs
// control.
struct AngelInput
{
    v2_i64 stick_1;
    v2_i64 stick_2;
    u16 buttons;
    u16 keys;
};

struct AngelInputArray 
{
    AngelInput inputs[MAX_DEVICES];
    u32 open_index; // either an index never assigned, or in a full array of inputs, the most recently cleared index.
};

// Gamepads.
#define DPAD_UP 1
#define DPAD_DOWN 2
#define DPAD_LEFT 4
#define DPAD_RIGHT 8
#define BUTTON_START 16
#define BUTTON_BACK 32
#define BUTTON_A 64
#define BUTOON_B 128
#define BUTTON_X 256
#define BUTTON_Y 512

#define STICK_DEADSPACE 4000

// Computer Keyboard.
#define KEY_W 8

static void GameUpdateAndRender(GameMemory* memory, GameOffscreenBuffer* buffer, std::vector<AngelInput> inputs);

// SOUND.
void SoundOutput();

// RENDERING.
void RenderModels(Obj* models, Camera camera);
bool IsTriangleInCamera(Tri* triangle, Camera camera, Transform tra);

static void CreateSoundTable();
#endif
