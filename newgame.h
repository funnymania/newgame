#ifndef NEWGAME_H
#define NEWGAME_H

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
void SoundOutput();

static void CreateSoundTable();
#endif
