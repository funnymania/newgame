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

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

struct v2_i64
{
    i64 x;
    i64 y;
};

struct v3 
{
    u64 x;
    u64 y;
    u64 z;
};

struct v3_float 
{
    f32 x;
    f32 y;
    f32 z;
};

struct v3_float_64
{
    f64 x;
    f64 y;
    f64 z;
};

struct Tri 
{
    v3_float verts[3];
    v3_float normals[3];
};

struct Ray 
{
    v3 begin;
    v3 end;
};

struct Transform 
{
    v3 pos;
    v3 rot;
    // v3 scale;
};

struct GameMemory 
{
    void* permanent_storage;
    void* transient_storage;
    u64 permanent_storage_size;
    u64 transient_storage_size;
    u64 permanent_storage_remaining;
    u64 transient_storage_remaining;
    u64* next_available;
};

struct Pair_u16 
{
    u16 one;
    u16 other;
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

