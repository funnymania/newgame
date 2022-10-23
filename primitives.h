#if !defined(PRIMITIVES_H)

// note: uint8_t, etc.
#include <stdint.h>

// note: NEWGAME_INTERNAL 0 - Public release. 1 - Developers.
//       NEWGAME_SLOW 0 - No slow code allowed. 1 - slow code welcome.
#if NEWGAME_SLOW
#define Assert(Expression) \
    if(!(Expression)) {*(int*)0 = 0;}
#else
#define Assert(Expression)
#endif

#define internal static
#define local_persist static
#define global_variable static

#define Kilobytes(Value) (Value * 1024)
#define Megabytes(Value) (Kilobytes(Value * 1024))
#define Gigabytes(Value) (Megabytes(Value * 1024))
#define Terabytes(Value) (Gigabytes(Value * 1024))
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define MAX_DEVICES 64

// study: Should we derive this ourselves, or something different besides this constant without a type?
#define PI 3.14159265358979323846

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

struct v2i64
{
    i64 x;
    i64 y;
};

struct v2f64
{
    f64 x;
    f64 y;
};

struct u64_f64
{
    u64 x;
    f64 y;
};

struct v3 
{
    u64 x;
    u64 y;
    u64 z;
};

struct v3i64
{
    i64 x;
    i64 y;
    i64 z;
};

struct v3_float 
{
    f32 x;
    f32 y;
    f32 z;
};

struct v3f64
{
    f64 x;
    f64 y;
    f64 z;
};

struct v4 
{
    u64 x;
    u64 y;
    u64 z;
    u64 t;
};

struct v4i64
{
    i64 x;
    i64 y;
    i64 z;
    i64 t;
};

// study: can we support greater color resolution? genuinely curious!
struct Color 
{
    u8 red;
    u8 green;
    u8 blue;
    u8 a;
};

struct TriangleRefVertices 
{
    v3f64* verts[3];
    v3f64* normals[3];
    v2f64* uvs[3];
};

struct Tri 
{
    v3f64 verts[3];
};

struct TriColorNoRef
{
    Tri triangle;
    Color color;
};

struct Line 
{
    v3f64 x;
    v3f64 y;
};

struct Transform 
{
    v3 pos;
    v3 rot;
    // v3 scale;
};

enum MorphStatus 
{
    NO_MORPH,
    SPHERE,
};

struct Obj 
{
    Tri* triangles; // Vertice positions are OFFSETS from tra. A value of (1, 0, 0) is to the left of wherever tra is.
    Transform tra;
    u64 triangles_len;
};

struct Camera 
{
    v3 pos;
    v3 direction;    
    float wideness;
};

struct GameMemory 
{
    bool initialized;
    void* permanent_storage; // Memory which persists from frame to frame.
    void* transient_storage; // note: just being occupied by DevOpStats
    u64 permanent_storage_size;
    u64 transient_storage_size;
    u64 permanent_storage_remaining;
    u64 transient_storage_remaining;
    u8* next_available;
    u8* next_available_transient;
};

static void AdjustMemory(u64 size, u64 number, GameMemory* game_memory, void** to_allocate);

struct Pair_u16 
{
    u16 one;
    u16 other;
};

// note: services that the game provides to the platform layer.
struct GameOffscreenBuffer
{
    void *memory;
    int width;
    int height;
    int bytes_per_pixel = 4;
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

struct FileReadResult {
    void* data;
    u32 data_len;
};

// RENDERING.
void RenderModels(Obj* models, Camera camera);
bool IsTriangleInCamera(Tri* triangle, Camera camera, Transform tra);

#define PRIMITIVES_H
#endif
