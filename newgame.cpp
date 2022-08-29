#include "newgame.h"
#include "asset_table.cpp"

// perf: should be a growable array, somewhat like Java's implementation.
// static AudioAsset loaded_sounds[10];
// static u32 loaded_sounds_len = 0;
// static AudioAsset* running_sounds[10];
static u32 running_sounds_len = 0;

static SoundAssetTable sound_assets = {};

static void RenderWeirdGradient(GameOffscreenBuffer *buffer, int x_offset, int y_offset) 
{
    int width = buffer->width;
    int height = buffer->height;

    int pitch = width * buffer->bytes_per_pixel;
    u8 *row = (u8 *)buffer->memory; 
    for (int y = 0; y < buffer->height; y += 1) {
        u8 *pixel = (u8 *)row;
        for (int x = 0; x < buffer->width; x += 1) {
            *pixel = (u8)(x - x_offset);
            ++pixel;

            *pixel = (u8)(y + y_offset);
            pixel += 1;

            *pixel = (u8)0;
            pixel += 1;

            *pixel = (u8)0;
            pixel += 1;
        }

        row += pitch;
    }
}

static void InitGame() 
{
    // CreateSoundTable();
}

static void GameUpdateAndRender(GameOffscreenBuffer* buffer)
{
    // study: should perhaps be running more often than renderer.
    // OutputSound();

    // study: always be given a buffer for each supported audio type. 
    if (running_sounds_len == 0) {
        SoundAssetTable* sound_ptr = &sound_assets;
        LoadSound("Eerie_Town.wav", &sound_ptr); // Points our SoundAssetTable to the new head.
        
        // AudioMessage play_sound = { PLAY, audio };
        StartPlaying("Eerie_Town.wav", &sound_assets);

        running_sounds_len += 1;
    } 

    RenderWeirdGradient(buffer, buffer->x_offset, buffer->y_offset);
}

enum AudioCode 
{
    PLAY,
    PAUSE,
    STOP,
    STOP_ALL
};

struct AudioMessage 
{
    AudioCode code;
    AudioAsset* data;
};

struct AudioMessageQueue 
{
    AudioMessage message[100];
};

static AudioMessageQueue audio_queue = {};

// note: (Windows) streams must match formats supported by device at least by channel number and sample rate.
// shipping: devices may disconnect or be in exclusive mode, requiring fallbacks and potentially notifications
//           for the user.
// static void OutputSound() 
// {
//     for (int counter = 0; counter < running_sounds_len; counter += 1) {
//         if (running_sounds[counter]->paused == false) {
//             if (running_sounds[counter]->speed < 0) { 
//                 // Reverse.
//                 running_sounds[counter]->data_remaining += used_frames * 6;
//             } else {
//                 // Fowards.
//                 running_sounds[counter]->data_remaining -= used_frames * 6;
//             }
// 
//             if (running_sounds[counter]->data_remaining <= 0 
//                 || running_sounds[counter]->data_remaining >= running_sounds[counter]->audio_data_size) {
//                 //todo: Send event saying to stop playing.
//             }
// 
//             if (running_sounds[counter]->speed < 0) {
//                 // Reverse.
//                 
//             } else {
//                 memcpy(running_sounds[counter].player_buffer, running_sounds[counter].audio_data_seek,
//                         buffer_frames_available * running_sounds[counter].bytes_per_frame);
//                 running_sounds[counter].audio_data_seek += running_sounds[counter].buffer_frames_available;
//             }
// 
//             // OS -> Asleep waiting for something to play.
//             //......
//             // LOOP
//             // OS -> Is awoken to play something.
//             // OS -> Gives buffer to Game. Says, "New buffer!"
//             // OS -> Goes asleep. 
//             // Game -> While in a holding pattern, receives buffer.
//             // Game -> Figures out what to fill the buffer with.
//             // Game -> "Done with that buffer!"
//             // REPEAT
//             
//             // Receives a message saying, "here is your buffer now!"
//             // Wait on a message that says, Are you finished with that buffer yet?
//             // To which we respond, yes!, and clear the message from our queue.
//             // in the platform code, we will release that locked buffer space, and bring over another buffer.
//             // study: Return buffer without WaitForSingleObject, using thread_id, or perhaps using an async
//             //        message system, where Windows sends a message to our audio_message_queue, and
//             //        we then clear that message and respond. 
//         }
//     }
// }

// receives buffers, with formats to know how to write....
static void ReceivedAudioBuffer(AudioAsset* audio)
{
     
}

// AudioClient... 
//      Being able to Play a song.
//      Being able to stop playing a song
//      To pause a song
//      I care about a song being in reverse
//      I might also be interested in the speed of a song.
