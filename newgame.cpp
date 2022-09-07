#include <stdint.h>
#include <vector>

#include "primitives.h"
#include "sequence.cpp"
#include "double_interpolated_pattern.cpp"
#include "geometry_m.h"
#include "area.cpp"

#include "newgame.h"
#include "platform_services.h"

static u32 running_sounds_len = 0;

static SoundAssetTable sound_assets = {};

// note: we should probably support local multiplayer.
static Input final_inputs[MAX_DEVICES];
static AngelInputArray angel_inputs = {};

// Object for dual-motor rumbling.
static DoubleInterpolatedPattern rumble_patterns[MAX_DEVICES] = {};

static Camera camera;

static Area current_area;

static int weird_gradient_x = 0;
static int weird_gradient_y = 0;

static void RenderWeirdGradient(GameOffscreenBuffer *buffer, int x_offset, int y_offset) 
{
    int pitch = buffer->width * buffer->bytes_per_pixel;
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

static void InitGame(GameMemory* game_memory, PlatformServices services) 
{
    // Init memory which we allocate everything from.
    game_memory->next_available = (u64*)game_memory->permanent_storage;

    camera = { {0, 0, 0}, {0, 0, 1}, 0.5 };
    game_memory->permanent_storage_remaining -= sizeof(camera);
    game_memory->next_available += sizeof(camera);

    current_area = LoadArea("Place", game_memory, services);

    // Persona4 style rumbling.
    f32 first_arr[3][2] = {{0,0}, {20000,30}, {0,60}};
    Sequence_f32 first = Sequence_f32::Create(first_arr, 3, game_memory);
    Sequence_f32 second = Sequence_f32::Create(first_arr, 3, game_memory);

    rumble_patterns[0] = DoubleInterpolatedPattern::Create(first, second);

    game_memory->initialized = true;
}

static void AssignInput(std::vector<AngelInput> inputs) 
{
    for (int i = angel_inputs.open_index; i < inputs.size(); i += 1) {
        AngelInput new_angel = inputs.at(i);
        angel_inputs.inputs[i] = new_angel; // Copy.
        angel_inputs.open_index += 1;

        // Default values for final_input.
        Input new_in;
        new_in.lr = &angel_inputs.inputs[i].stick_1;
        new_in.z = &angel_inputs.inputs[i].stick_2;
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
        angel_inputs.inputs[i].stick_1 = inputs[i].stick_1;
        angel_inputs.inputs[i].stick_2 = inputs[i].stick_2;
        angel_inputs.inputs[i].keys = inputs[i].keys;

        // buffer->x_offset += (u32)(2 * final_inputs[i].lr->x / 32768);
        // buffer->y_offset += (u32)(2 * final_inputs[i].lr->y / 32768);

        // This is how we check if some game defined behavior is occcuring.
        u16 interact_down = angel_inputs.inputs[i].keys & final_inputs[i].interact_mask;
        if (interact_down == final_inputs[i].interact_mask) {
            // Switch left and right control stick. 
            final_inputs[i].lr = &angel_inputs.inputs[i].stick_2;
            final_inputs[i].z = &angel_inputs.inputs[i].stick_1;
        }

        // todo: area command parsing. if keyboard is L_KEY (HELD DOWN) + NUMBER (TYPED OUT), and then LET GO OF
        //       L_KEY, switch to Level_NUMBER.
#if NEWGAME_INTERNAL
        
#endif
    }
}

extern "C" void PauseAudio(char* name, PlatformServices services) 
{
    services.pause_audio(name, &sound_assets);
}

extern "C" void ResumeAudio(char* name, PlatformServices services) 
{
    services.resume_audio(name, &sound_assets);
}

extern "C" void GameUpdateAndRender(GameOffscreenBuffer* buffer, std::vector<AngelInput> inputs, 
        GameMemory* platform_memory, PlatformServices services)
{
    GameMemory* game_memory = platform_memory;
    if (game_memory->initialized == false) {
       InitGame(game_memory, services);
    }

    if (rumble_patterns[0].current_time <= 59) {
        services.persona_handshake(&rumble_patterns[0]);
    }

    // study: should perhaps be running more often than renderer.
    // OutputSound();

    // study: always be given a buffer for each supported audio type. 
    if (running_sounds_len == 0) {
        SoundAssetTable* sound_ptr = &sound_assets;
        services.load_sound("Eerie_Town.wav", &sound_ptr, game_memory); // Points our SoundAssetTable to the new head.
        
        // AudioMessage play_sound = { PLAY, audio };
        services.start_playing("Eerie_Town.wav", &sound_assets, game_memory);

        running_sounds_len += 1;
    } 

    AssignInput(inputs);
    ProcessInputs(buffer, inputs);
    weird_gradient_x += (u32)(2 * final_inputs[0].lr->x / 32768);
    weird_gradient_y += (u32)(2 * final_inputs[0].lr->y / 32768);
    RenderWeirdGradient(buffer, weird_gradient_x, weird_gradient_y);
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
