#ifndef NEWGAME_H
#define NEWGAME_H

// note: NEWGAME_INTERNAL 0 - Public release. 1 - Developers.
//       NEWGAME_SLOW 0 - No slow code allowed. 1 - slow code welcome.
#if NEWGAME_SLOW
#define Assert(Expression) \
    if(!(Expression)) {*(int*)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) (Value * 1024)
#define Megabytes(Value) (Kilobytes(Value * 1024))
#define Gigabytes(Value) (Megabytes(Value * 1024))
#define Terabytes(Value) (Gigabytes(Value * 1024))
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define MAX_DEVICES 64

struct GameMemory 
{
    void* permanent_storage;
    void* transient_storage;
    u64 permanent_storage_size;
    u64 transient_storage_size;
    u64 permanent_storage_remaining;
    u64 transient_storage_remaining;
    u64* next_available;
    bool initialized;
};

struct Pair_u16 
{
    u16 one;
    u16 other;
};

// List of 2-arrays
struct Sequence_f32 
{
    f64* arr;
    u64 size;

    Sequence_f32() {}

    Sequence_f32(f32 array[][2], int arr_size, GameMemory* game_memory) 
    {
        arr = (f64*)game_memory->next_available;

        f64* seek = arr;
        // init arr for arr_size.
        for (int counter = 0; counter < arr_size; counter += 1) {
            for (int counter2 = 0; counter2 < 2; counter2 += 1) {
                *seek = (f64)array[counter][counter2];
                game_memory->permanent_storage_remaining -= 8;
                game_memory->next_available += 8;
                seek += 1;
            }
        }

        size = arr_size;
    }
};

struct DoubleInterpolatedPattern
{
    f32 value[60];    // where '60' is frames (aka time)
    f32 value_2[60];  // where '60' is frames (aka time)
    u16 current_time; // starts at 0, ends at '60'
                      
    DoubleInterpolatedPattern() {}

    // note: sequence must always specific a starting and ending value.
    static DoubleInterpolatedPattern Create(Sequence_f32 sequence, Sequence_f32 sequence2) 
    {
        DoubleInterpolatedPattern result = {};

        // init to -1.
        for (int i = 0; i < 60; i += 1) {
            result.value[i] = -1;
            result.value_2[i] = -1;
        }

        // Compute the full values.
        for (int i = 0; i < sequence.size - 1; i += 1) {
            int current_value = (&sequence.arr[2*i])[0];
            int next_value = (&sequence.arr[2 * (i + 1)])[0];

            int current_time = (&sequence.arr[2*i])[1];
            int next_time = (&sequence.arr[2 * (i + 1)])[1];
            
            for (int j = current_time; j < next_time; j += 1) {
                result.value[j] = 
                    current_value + ((next_value - current_value) / (next_time - current_time) * (j - current_time));
            }

            result.value[next_time] = (&sequence.arr[2 * (i + 1)])[0];
        }

        for (int i = 0; i < sequence2.size - 1; i += 1) {
            int current_time = (&sequence2.arr[2*i])[1];
            int next_time = (&sequence2.arr[2 * (i + 1)])[1];

            int current_value = (&sequence2.arr[2*i])[0];
            int next_value = (&sequence2.arr[2*(i + 1)])[0];
            
            for (int j = current_time; j < next_time; j += 1) {
                result.value_2[j] = 
                    current_value + ((next_value - current_value) / (next_time - current_time) * (j - current_time));
            }

            result.value_2[next_time] = (&sequence2.arr[2*(i + 1)])[0];
        }

        return(result);
    }
};

// note: services that the game provides to the platform layer.
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

// DEPRECATED.
// Represents a single file.
// struct StaticSoundAsset {
//     char* name;
//     StaticAudioStream* stream; 
//     DWORD thread_id;
//     // StaticSoundAsset* next;
//     u8* media_ptr;
// };

// study: How to do lookup table given manual memory management.
struct SoundAssetTable {
    AudioAsset value;
    SoundAssetTable* next;
};

static SoundAssetTable* Add(SoundAssetTable* table, AudioAsset asset, GameMemory* game_memory);
static AudioAsset* Get(SoundAssetTable* table, char* name);

void LoadSound(char* file_name);
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
