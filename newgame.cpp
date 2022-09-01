#include "newgame.h"
#include "asset_table.cpp"
#include "area.cpp"

// static AudioAsset loaded_sounds[10];
// static u32 loaded_sounds_len = 0;
// static AudioAsset* running_sounds[10];
static u32 running_sounds_len = 0;

static SoundAssetTable sound_assets = {};

// note: we should probably support local multiplayer.
static Input final_inputs[MAX_DEVICES];
static AngelInputArray angel_inputs = {};
static GameMemory game_memory = {};

// static RunningModels runtime_models;
static Camera camera;

static Area current_area;

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
    camera = { {0, 0, 0}, {0, 0, 1}, 0.5 };
    game_memory.transient_storage_remaining -= sizeof(camera);

    current_area = LoadArea("Place", &game_memory);
    
}

static void AssignInput(std::vector<AngelInput> inputs) 
{
    for (int i = angel_inputs.open_index; i < inputs.size(); i += 1) {
        AngelInput new_angel = inputs.at(i);
        angel_inputs.inputs[i] = new_angel; // This is copy semantics! So this is a thing that RUST can improve upon.
        angel_inputs.open_index += 1;

        // If this were Rust:
        // AngelInput new_angel = inputs.at(i);
        // angel_inputs[i] = AngelInput { ..new_angel }; // This is EXPLICIT. It is NOT a reference, it is a new 
        //                                                //     struct.
        // We CAN accomplish as we did above in c++, IF we derive or implement our own support on the AngelInput struct
        //       specifically FOR copy semantics.

        // Default values for final_input.
        Input new_in;
        new_in.lr = &angel_inputs.inputs[i].lr;
        new_in.z = &angel_inputs.inputs[i].z;
        new_in.interact_mask = 8;
        final_inputs[i] = new_in;
    }
}

static void ConfigScreen() 
{
    // move around.... interact with an input field.
    // toggle some button/stick. 
    //       whatever I see positive input from, this is what is being pointed to by that field.
    //       Ex. select move left-right field, use the Dpad, I will assign the memory address there
    //           to be what is used for moving left-right for you.
    // option to reset to default.
}


static void ProcessInputs(GameOffscreenBuffer *buffer, std::vector<AngelInput> inputs) 
{
    for (int i = 0; i < angel_inputs.open_index; i += 1) { 
        angel_inputs.inputs[i].lr = inputs[i].lr;
        angel_inputs.inputs[i].z = inputs[i].z;
        angel_inputs.inputs[i].keys = inputs[i].keys;

        buffer->x_offset += 2 * final_inputs[i].lr->x / 32768;
        buffer->y_offset += 2 * final_inputs[i].lr->y / 32768;

        int interact_down = angel_inputs.inputs[i].keys & final_inputs[i].interact_mask;
        if (interact_down == final_inputs[i].interact_mask) {
            // Switch left and right control stick. 
            final_inputs[i].lr = &angel_inputs.inputs[i].z;
            final_inputs[i].z = &angel_inputs.inputs[i].lr;
        }
    }
}

static void GameUpdateAndRender(GameMemory* memory, GameOffscreenBuffer* buffer, std::vector<AngelInput> inputs)
{
    if (game_memory.initialized == false) {
        game_memory = *memory; // copy.
        InitGame();
        game_memory.initialized = true;
    }

    // study: should perhaps be running more often than renderer.
    // OutputSound();

    // study: always be given a buffer for each supported audio type. 
    if (running_sounds_len == 0) {
        SoundAssetTable* sound_ptr = &sound_assets;
        LoadSound("Eerie_Town.wav", &sound_ptr, &game_memory); // Points our SoundAssetTable to the new head.
        
        // AudioMessage play_sound = { PLAY, audio };
        StartPlaying("Eerie_Town.wav", &sound_assets, &game_memory);

        running_sounds_len += 1;
    } 

    AssignInput(inputs);
    ProcessInputs(buffer, inputs);
    RenderWeirdGradient(buffer, buffer->x_offset, buffer->y_offset);
    
    // todo: render the cube!
    // RenderModels(&current_area.models, camera);
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

// note: reserved for multi-threaded independence.
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
