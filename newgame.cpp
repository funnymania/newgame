#include <stdint.h>
#include <vector>
#include <math.h>

#include "primitives.h"
#include "list.cpp"
#include "sequence.cpp"
#include "double_interpolated_pattern.cpp"
#include "interpolated_pattern.cpp"
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

static Area current_area;

static i64 weird_gradient_x = 0;
static i64 weird_gradient_y = 0;
static i64 weird_gradient_z = 0;

// perf: frame drops go away when compiler option -Og (or its super-set -O2) are declared.
//       this is doing loop optimization, and perhaps some more things. 
static void Render(GameOffscreenBuffer *buffer, i64 x_offset, i64 y_offset, i64 z_offset) 
{
    // idea: 256 is z=0. z = -256 would be 0 pixels wide. z = 256 would be 512
    int pitch = buffer->width * buffer->bytes_per_pixel;
    u8 *row = (u8 *)buffer->memory; 
    for (i32 y = 0; y < buffer->height; y += 1) {
        u32 *pixel = (u32*)row;
        for (i32 x = 0; x < buffer->width; x += 1) {
            i32 g = (i32)(z_offset / GRID_SIZE + 256);
            f32 gf = 256.0f / g; 

            i32 value = (i32)((x + x_offset / GRID_SIZE) * gf);
            value = value << 8;
            value += (i32)((y - y_offset / GRID_SIZE) * gf);

            *pixel = value;

            pixel += 1;
        }

        row += pitch;
    }
}

// note: angel_inputs and final_inputs are global.
static void SetDefaultInputs(std::vector<AngelInput> inputs)
{
    for (int i = 0; i < inputs.size(); i += 1) {
        AngelInput new_angel = inputs.at(i);
        angel_inputs.inputs[i] = new_angel; // Copy.

        // Default values for final_input.
        Input new_in;
        new_in.lr = &angel_inputs.inputs[i].stick_1;
        new_in.z = &angel_inputs.inputs[i].stick_2;
        new_in.interact_mask = 8;
        final_inputs[i] = new_in;
    }

    angel_inputs.next_available_index = (u32)inputs.size();
}

static void InitGame(GameMemory* game_memory, PlatformServices services, std::vector<AngelInput> inputs) 
{
    // Start from initial area. 
    current_area = LoadArea(0, game_memory, services, &rumble_patterns[0]);

    SetDefaultInputs(inputs);
    game_memory->initialized = true;
}

static int NumberKeyFirstDown(int bitflag) 
{
    int result = -1;

    if (bitflag & KEY_0) {
        result = 0;
    } else if (bitflag & KEY_1) {
        result = 1;
    } else if (bitflag & KEY_2) {
        result = 2;
    } else if (bitflag & KEY_3) {
        result = 3;
    } else if (bitflag & KEY_4) {
        result = 4;
    } else if (bitflag & KEY_5) {
        result = 5;
    } else if (bitflag & KEY_6) {
        result = 6;
    } else if (bitflag & KEY_7) {
        result = 7;
    } else if (bitflag & KEY_8) {
        result = 8;
    } else if (bitflag & KEY_9) {
        result = 9;
    }

    return(result);
}

inline int Calculate10DigitValue(int base, int digit_number)
{
    return(int)(base * pow(10, digit_number));
}

inline bool IsKeyDown(u32 key, u32 device_id) 
{
    return((angel_inputs.inputs[device_id].keys & key) == key);
}

inline bool IsKeyFirstDown(u32 key, u32 device_id)
{
    return((angel_inputs.inputs[device_id].keys_first_down & key) == key);
}

inline bool IsKeyFirstUp(u32 key, u32 device_id)
{
    return((angel_inputs.inputs[device_id].keys_first_up & key) == key);
}

// study: group as something to reference in developer code.
static int dev_stage_id;
static int digit_number = 0;
static bool paused;

internal void ProcessInputs(GameOffscreenBuffer *buffer, std::vector<AngelInput> inputs, GameMemory* game_memory,
        PlatformServices services) 
{
    for (int i = 0; i < angel_inputs.next_available_index; i += 1) {
        angel_inputs.inputs[i].stick_1 = inputs[i].stick_1;
        angel_inputs.inputs[i].stick_2 = inputs[i].stick_2;
        angel_inputs.inputs[i].keys = inputs[i].keys;
        angel_inputs.inputs[i].keys_first_down = inputs[i].keys_first_down;
        angel_inputs.inputs[i].keys_first_up = inputs[i].keys_first_up;

        // This is how we check if some game defined behavior is occcuring.
        u16 interact_down = angel_inputs.inputs[i].keys & final_inputs[i].interact_mask;
        if (interact_down == final_inputs[i].interact_mask) {
            // Switch left and right control stick. 
            final_inputs[i].lr = &angel_inputs.inputs[i].stick_2;
            final_inputs[i].z = &angel_inputs.inputs[i].stick_1;
        }

        // note: keys are little endian! this means 'L12' means Load 21.
#if NEWGAME_INTERNAL
        if (i == 0) {
            if (IsKeyDown(KEY_L, i)) {
                // If numeric key down, continue adding up the following numbers.
                // note: we just ignore if you hit non-numeric keys while L is held down.
                int digit_held = NumberKeyFirstDown(angel_inputs.inputs[i].keys_first_down);
                if (digit_held != -1) {
                    dev_stage_id += Calculate10DigitValue(digit_held, digit_number);
                    digit_number += 1;
                }
            } else if (IsKeyFirstUp(KEY_L, i)) {
                LoadArea(dev_stage_id, game_memory, services, &rumble_patterns[0]);
                dev_stage_id = 0;
                digit_number = 0;
            }
            bool result = IsKeyFirstUp(KEY_P, i);
            if (result) {
                // Pause the game. 
                paused = !paused;
            }
        }
#endif
    }
}

// note: To play a song. Loads the song if it is not yet loaded. 
void PlayASong(char* name, GameMemory* game_memory, PlatformServices services) 
{
    SoundAssetTable* sound_ptr = &sound_assets;
    services.load_sound("Eerie_Town.wav", &sound_ptr, game_memory); // Points our SoundAssetTable to the new head.
    
    services.start_playing("Eerie_Town.wav", &sound_assets, game_memory);
}

// note: to rumble controllers, pass a pointer to the pattern. Pattern will be mutated platform side, so make sure 
//       to send the same pattern data whenever you need to continue rumbling. 
void RumbleController(u32 device_index, DoubleInterpolatedPattern* pattern, PlatformServices services)
{
    services.rumble_controller(device_index, pattern);
}

extern "C" void GameUpdateAndRender(GameOffscreenBuffer* buffer, std::vector<AngelInput> inputs, 
        GameMemory* platform_memory, PlatformServices services)
{
    GameMemory* game_memory = platform_memory;
    if (game_memory->initialized == false) {
        InitGame(game_memory, services, inputs);
    }

    ProcessInputs(buffer, inputs, game_memory, services);
    LevelResults* demo = ExecuteLevel();

    if (paused == false) {
        // note: everything in here is what we want to be pausable.
        if (demo->path.current_time < demo->path.length) {
            weird_gradient_x = Get(demo->path.values, demo->path.current_time)->x;
            weird_gradient_y = Get(demo->path.values, demo->path.current_time)->y;
            weird_gradient_z = Get(demo->path.values, demo->path.current_time)->z;
            demo->path.current_time += 1;
        }
    }
    
    Render(buffer, weird_gradient_x, weird_gradient_y, weird_gradient_z);
}

extern "C" void PauseAudio(char* name, PlatformServices services) 
{
    services.pause_audio(name, &sound_assets);
}

extern "C" void ResumeAudio(char* name, PlatformServices services) 
{
    services.resume_audio(name, &sound_assets);
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
