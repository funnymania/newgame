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
    HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
    if (XInputLibrary) {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

// perf: memory churn from clears.
internal void GetDeviceInputs(std::vector<AngelInput>* result, bool* program_running) 
{
    result->clear();
    std::vector<XINPUT_GAMEPAD> inputs;
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i += 1) {
        XINPUT_STATE device_state;
        AngelInput angelic = {};

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
                angelic.stick_1.x = pad.sThumbLX;
            } else {
                angelic.stick_1.x = 0;
            }

            if (pad.sThumbLY > STICK_DEADSPACE || pad.sThumbLY < STICK_DEADSPACE * -1) {
                angelic.stick_1.y = pad.sThumbLY;
            } else {
                angelic.stick_1.y = 0;
            }

            // Z
            if (pad.sThumbRX > STICK_DEADSPACE || pad.sThumbRX < STICK_DEADSPACE * -1) {
                angelic.stick_2.x = pad.sThumbRX;
            } else {
                angelic.stick_2.x = 0;
            }
            if (pad.sThumbRY > STICK_DEADSPACE || pad.sThumbRY < STICK_DEADSPACE * -1) {
                angelic.stick_2.y = pad.sThumbRY;
            } else {
                angelic.stick_2.y = 0;
            }

            angelic.buttons = pad.wButtons;
        } else {
            // Controller is not connected
        }

        // Get keyboard inputs.
        if (GetAsyncKeyState(87) & 0x01) {
            // buffer.y_offset += -2;
            // Pause("Eerie_Town.wav");
            // Reverse("Eerie_Town.wav");
            angelic.keys ^= KEY_W;
        }

        if (GetAsyncKeyState(65) & 0x01) {
            // buffer.x_offset += 2;
            // Unpause("Eerie_Town.wav");
            angelic.keys ^= KEY_A;
        }

        if (GetAsyncKeyState(83) & 0x01) {
            // buffer.y_offset += 2;
            // StopPlaying("Eerie_Town.wav");
            angelic.keys ^= KEY_S;
        }

        if (GetAsyncKeyState(68) & 0x01) {
            // buffer.x_offset += -2;
            angelic.keys ^= KEY_D;
        }

        // Alt + F4
        if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState(VK_F4)) {
            program_running = false;
        }

        result->push_back(angelic);
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
