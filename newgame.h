#ifndef NEWGAME_H
#define NEWGAME_H

#define MAX_DEVICES 64

// note: services that the game provides to the platform layer.

// Timer, Device input, bitmap buffer to use, sound buffer to use.
struct GameOffscreenBuffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int bytes_per_pixel = 4;
    int x_offset = 0; 
    int y_offset = 0; 
};

// Audio which is loaded into faster memory.
struct AudioAsset 
{
    char name[32];
    u8* audio_data;
    u32 audio_data_size;
    u8* audio_data_seek;
    u8* player_buffer;
    u32 buffer_frames_available;
    int speed;
    int channels;
    u32 sample_rate;
    int bits; 
    int bytes_per_frame; // channels * bits / 8
    int frames_per_buffer; // passed by platform
    i32 data_remaining;
    bool paused;
};

struct SoundPlayResult 
{
    u32 response;  // either 0 for error, or 1 for success.
};

// Represents a single file.
// struct StaticSoundAsset {
//     char* name;
//     StaticAudioStream* stream; 
//     DWORD thread_id;
//     // StaticSoundAsset* next;
//     u8* media_ptr;
// };

// todo: should be a lookup table, not linked list.
// implement as an array (or, learn about cpp stuff?)
struct SoundAssetTable {
    AudioAsset value;
    SoundAssetTable* next;
};

static SoundAssetTable* Add(SoundAssetTable* table, AudioAsset asset);
static AudioAsset* Get(SoundAssetTable* table, char* name);

void LoadSound(char* file_name);
SoundPlayResult StartPlaying(char* name);

void GameUpdateAndRender(GameOffscreenBuffer* buffer);

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
    v2_i64 lr;
    v2_i64 z;
    u16 buttons;
    u16 keys;
};

struct AngelInputArray 
{
    AngelInput inputs[MAX_DEVICES];
    u32 open_index;
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


// WE DONT CARE.
// On some user config screen. Get device input tags. Receive events on new device connected? it would only be 
//       relevant on this screen.
//
// WE DONT CARE.
// Allow user to assign the above inputs to these input tags.
//
// WE DONT CARE.
// Platform writes some inputs tagged up.
//
// WE CARE.
// Game will use UserInputConfig to determine what this corresponds to.

// WE CARE about default mapping. So, control sticks can default map. WASD, and arrows can default map.
// We can say on the platform side that sThumbLX and sThumbLY are default InputResult.one.
// We can also say that WASD is also. 


























// SOUND.
void SoundOutput();

// RENDERING.
struct RunningModels 
{
    Obj* models;
    u64 models_len;
};
void RenderModels(RunningModels* models, Camera camera);
bool IsTriangleInCamera(Tri* triangle, Camera camera, Transform tra);

static void CreateSoundTable();
#endif
