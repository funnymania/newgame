#include "newgame.h"

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return(0);
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pState)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
    return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void) 
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (XInputLibrary) {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

inline bool Win32KeyDown(int key)
{
    return((GetAsyncKeyState(key) & 0x8000) == 0x8000);
}

internal void Win32InitInputArray(std::vector<AngelInput>* result) 
{
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i += 1) {
        AngelInput angelic = {};
        result->push_back(angelic);
    }
}

internal inline void Win32ControllerDisconnected(std::vector<AngelInput>* result, DWORD index) 
{
    (*result).at(index).stick_1.x = 0;
    (*result).at(index).stick_1.y = 0;

    (*result).at(index).stick_2.x = 0;
    (*result).at(index).stick_2.y = 0;
}

internal inline void Win32ZeroButtonFirstState(std::vector<AngelInput>* result) 
{
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i += 1) {
        (*result).at(i).keys_first_down = 0;
        (*result).at(i).keys_first_up = 0;
    }
}

global_variable List<Win32KeycodeMap> important_keys;

internal inline void Win32InitKeycodeMapping(GameMemory* game_memory)
{
    // Initialize it.
    important_keys = {};
    important_keys.array = (Win32KeycodeMap*)game_memory->next_available; 

    // Add to it. 
    AddToList<Win32KeycodeMap>(&important_keys, { 87, KEY_W }, game_memory); // W
    AddToList<Win32KeycodeMap>(&important_keys, { 65, KEY_A }, game_memory); // A
    AddToList<Win32KeycodeMap>(&important_keys, { 83, KEY_S }, game_memory); // S
    AddToList<Win32KeycodeMap>(&important_keys, { 68, KEY_D }, game_memory); // D
    AddToList<Win32KeycodeMap>(&important_keys, { 76, KEY_L }, game_memory); // L
    AddToList<Win32KeycodeMap>(&important_keys, { 48, KEY_0 }, game_memory); // 0
    AddToList<Win32KeycodeMap>(&important_keys, { 49, KEY_1 }, game_memory); 
    AddToList<Win32KeycodeMap>(&important_keys, { 50, KEY_2 }, game_memory);
    AddToList<Win32KeycodeMap>(&important_keys, { 51, KEY_3 }, game_memory);
    AddToList<Win32KeycodeMap>(&important_keys, { 52, KEY_4 }, game_memory);
    AddToList<Win32KeycodeMap>(&important_keys, { 53, KEY_5 }, game_memory);
    AddToList<Win32KeycodeMap>(&important_keys, { 54, KEY_6 }, game_memory);
    AddToList<Win32KeycodeMap>(&important_keys, { 55, KEY_7 }, game_memory);
    AddToList<Win32KeycodeMap>(&important_keys, { 56, KEY_8 }, game_memory);
    AddToList<Win32KeycodeMap>(&important_keys, { 57, KEY_9 }, game_memory); // 9
}

internal inline void Win32ReceiveKeyboardStates(std::vector<AngelInput>* result, DWORD index) 
{
    Win32KeycodeMap* tmp = important_keys.array;
    for (int counter = 0; counter < important_keys.size; counter += 1) {
        if (tmp->win32_key_code == 76) {
            int banana = 4;
        }
        if (Win32KeyDown(tmp->win32_key_code)) {
            (*result).at(index).keys |= tmp->game_key_code;
        } else {
            (*result).at(index).keys &= tmp->game_key_code ^ 0xFFFFFFFF;
        }

        if (((*result).at(index).keys & tmp->game_key_code) == tmp->game_key_code
            && ((*result).at(index).keys_held_down & tmp->game_key_code) == 0) 
        {
            (*result).at(index).keys_first_down |= tmp->game_key_code; 
            (*result).at(index).keys_held_down |= tmp->game_key_code;
        } 
        else if (((*result).at(index).keys & tmp->game_key_code) == 0) 
        {
            if (((*result).at(index).keys_held_down & tmp->game_key_code) == tmp->game_key_code) {
                (*result).at(index).keys_held_down &= tmp->game_key_code ^ 0xFFFFFFFF;
                (*result).at(index).keys_first_up |= tmp->game_key_code;
            }
        }

        tmp += 1;
    }
}

internal void GetDeviceInputs(std::vector<AngelInput>* result, bool* program_running) 
{
    // Zero out the buttons' initial down/up states.
    Win32ZeroButtonFirstState(result);
    std::vector<XINPUT_GAMEPAD> inputs;
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i += 1) {
        XINPUT_STATE device_state;

        DWORD dwResult = XInputGetState(i, &device_state);
        if (dwResult == ERROR_SUCCESS) {
            // Simply get the state of the controller from XInput.
            // note: Controller is connected
            XINPUT_GAMEPAD pad = device_state.Gamepad;

            // All I care about are the default (control sticks, WASD).
            // But this can be overwritten by the user, and then whatever the USER says is all I care about
            // henceforth. 
            // This user preference should be written to file. 
            // Default should just be here in the executable.

            // LR/FB
            if (pad.sThumbLX > STICK_DEADSPACE || pad.sThumbLX < STICK_DEADSPACE * -1) {
                (*result).at(i).stick_1.x = pad.sThumbLX;
            } else {
                (*result).at(i).stick_1.x = 0;
            }

            if (pad.sThumbLY > STICK_DEADSPACE || pad.sThumbLY < STICK_DEADSPACE * -1) {
                (*result).at(i).stick_1.y = pad.sThumbLY;
            } else {
                (*result).at(i).stick_1.y = 0;
            }

            // Z
            if (pad.sThumbRX > STICK_DEADSPACE || pad.sThumbRX < STICK_DEADSPACE * -1) {
                (*result).at(i).stick_2.x = pad.sThumbRX;
            } else {
                (*result).at(i).stick_2.x = 0;
            }
            if (pad.sThumbRY > STICK_DEADSPACE || pad.sThumbRY < STICK_DEADSPACE * -1) {
                (*result).at(i).stick_2.y = pad.sThumbRY;
            } else {
                (*result).at(i).stick_2.y = 0;
            }

            (*result).at(i).buttons = pad.wButtons;
        } else {
            // Controller is not connected, zero its values.
            Win32ControllerDisconnected(result, i);
        }

        // Get keyboard inputs.
        Win32ReceiveKeyboardStates(result, i);

        // Alt + F4
        if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState(VK_F4)) {
            program_running = false;
        }
    }
}

void Persona4Handshake(DoubleInterpolatedPattern* pattern) 
{
    XINPUT_VIBRATION vibration;
    ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

    // 0-65535, lo motor
    vibration.wLeftMotorSpeed = (WORD)pattern->value[pattern->current_time];
    vibration.wRightMotorSpeed = (WORD)pattern->value_2[pattern->current_time];      

    pattern->current_time += 1;

    // vibration.wRightMotorSpeed = 16000;  // 0-65535, hi motor
    XInputSetState(0, &vibration);
}
