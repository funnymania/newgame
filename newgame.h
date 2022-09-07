#ifndef NEWGAME_H
#define NEWGAME_H

// note: services that the platform provides to the game.
#if NEWGAME_INTERNAL
void PlatformReadEntireFile(char* file_name, file_read_result* file_result, GameMemory* game_memory);
void PlatformFreeFileMemory(void* memory);
bool PlatformWriteEntireFile(char* file_name, u32 data_size, void* data);
#endif

// study: How to do lookup table given manual memory management.
struct SoundAssetTable {
    AudioAsset value;
    SoundAssetTable* next;
};

typedef void load_sound(char* file_name, SoundAssetTable** sound_assets, GameMemory* game_memory);
typedef SoundPlayResult start_playing(char* name, SoundAssetTable* sound_assets, GameMemory* game_memory);

typedef SoundPlayResult pause_audio_game(char* name, SoundAssetTable* sound_assets);
typedef SoundPlayResult resume_audio_game(char* name, SoundAssetTable* sound_assets);

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

// MSFT recommends around 7800 for the left stick, around 8200 for the right. I don't know why. This is the value I
// have set on both for now.
#define STICK_DEADSPACE 7600

// Computer Keyboard.
#define KEY_A 1
#define KEY_S 2
#define KEY_D 4
#define KEY_W 8

static void GameUpdateAndRender(GameOffscreenBuffer* buffer, std::vector<AngelInput> inputs, GameMemory* memory);

// SOUND.
void SoundOutput();

static void CreateSoundTable();
#endif
