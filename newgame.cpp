#include <windows.h>
#include <xinput.h>
#include <stdint.h>
#include <vector>

#include "geometry_m.h"
#include "allocator.cpp"

#define internal static
#define local_persist static
#define global_variable static

struct win32_offscreen_buffer 
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int bytes_per_pixel = 4;
    int x_offset = 0; 
    int y_offset = 0; 
};

struct win32_window_dimensions 
{
    int width;
    int height; 
};

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

// todo: global to be removed later.
global_variable bool Running;
global_variable win32_offscreen_buffer global_back_buffer;

global_variable Obj runtime_models;

internal void Win32LoadXInput(void) 
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
    if (XInputLibrary) {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

internal win32_window_dimensions GetWindowDimension(HWND window) 
{
    win32_window_dimensions result;

    RECT client_rect;
    GetClientRect(window, &client_rect);

    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;

    return(result);
}

internal void RenderWeirdGradient(win32_offscreen_buffer *buffer, int x_offset, int y_offset) 
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


// note: callled on WM_SIZE.
internal void Win32ResizeDIBSection(win32_offscreen_buffer *buffer, LONG width, LONG height) 
{
    if (buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmap_memory_size = buffer->bytes_per_pixel * buffer->width * buffer->height;
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

    RenderWeirdGradient(buffer, buffer->x_offset, buffer->y_offset);
}

internal std::vector<XINPUT_GAMEPAD> GetDeviceInputs() 
{
    std::vector<XINPUT_GAMEPAD> inputs;
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i += 1) {
        XINPUT_STATE device_state;

        // Simply get the state of the controller from XInput.
        DWORD dwResult = XInputGetState(i, &device_state);
        if (dwResult == ERROR_SUCCESS) {
            // note: Controller is connected
            XINPUT_GAMEPAD pad = device_state.Gamepad;

            inputs.push_back(pad);
        } else {
            // Controller is not connected
        }
    }

    return(inputs);
}

internal void ProcessInputs(win32_offscreen_buffer *buffer, std::vector<XINPUT_GAMEPAD> inputs) 
{
    for (int i = 0; i < inputs.size(); i += 1) {
        if (inputs.at(i).sThumbLX > 5000) {
            buffer->x_offset += 1;
        } else if (inputs.at(i).sThumbLX < -5000) {
            buffer->x_offset += -1;
        } else  {
            // buffer->x_offset = 0;
        }

        if (inputs.at(i).sThumbLY > 5000) {
            buffer->y_offset += 1;
        } else if (inputs.at(i).sThumbLY< -5000) {
            buffer->y_offset += -1;
        } else {
            // buffer->y_offset = 0;
        }
    }
}

// note: callled on WM_PAINT.
internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *buffer, 
        HDC DeviceContext, int X, int Y, int width, int height) 
{
    StretchDIBits(DeviceContext, 0, 0, width, height, 0, 0, buffer->width, buffer->height, buffer->memory, 
            &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT Win32MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
) {
    LRESULT Result = 0;

    //TODO: Render when User is moving window.
    switch(Message) {
        case WM_SIZE: 
        {
            win32_window_dimensions dimensions = GetWindowDimension(Window);
            Win32ResizeDIBSection(&global_back_buffer, dimensions.width, dimensions.height);
            // TODO: Render things HERE. 
        } break;

        case WM_DESTROY: 
        {
            Running = false;
        } break;

        case WM_ACTIVATEAPP: 
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        {
            
        } break;

        case WM_SYSKEYUP:
        {
            
        } break;

        case WM_KEYDOWN:
        {
            // W.
//             if (GetKeyState(87) >= 0) {
//                global_back_buffer.y_offset += -1;
//             }
// 
//             // A.
//             if (GetKeyState(65) >= 0) {
//                 global_back_buffer.x_offset += 1;
//             }
// 
//             // S.
//             if (GetKeyState(83) >= 0) {
//                 global_back_buffer.y_offset += 1;
//             }
// 
//             // D.
//             if (GetKeyState(68) >= 0) {
//                 global_back_buffer.x_offset += -1;
//             }
        } break;

        case WM_KEYUP:
        {
            u32 vk_code = WParam;
        } break;

        case WM_PAINT: 
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            
            INT X = Paint.rcPaint.left;
            INT Y = Paint.rcPaint.top;
            LONG Width = Paint.rcPaint.right - Paint.rcPaint.left;
            LONG height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            Win32DisplayBufferInWindow(&global_back_buffer, DeviceContext, X, Y, Width, height);
            EndPaint(Window, &Paint);
        } break;

        default: 
        {
            OutputDebugStringA("default\n");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

void RenderModels(Obj* models) 
{
    /* Uncomment for models.
     *
     * TODO: Some table for storing allocated memory for models.
     * WhatThingsAreInCamera()
     * SendToGPU();
    */

    // Downward, angled towards viewer to their left.
    v3_float light = {};
    light.x = -1;
    light.y = -1;
    light.z = 1;

    // for each model.....
    // for each triangle.....
    // 1. if tri is in camera view, we need to render it. 
    // 2. how far is it (x, y, and z) from camera? (this determines where on the screen it is, and how much space it 
    //       takes up.
    // 3. cross multiply tri with camera to determine the camera_view_shape of tri.
    // 4. "shrink" size relative to z distance from camera.
    
    // calculate lighting to determine what each triangle should be colored as.
    
    // paint that imagery.
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR pCmdLine, int nCmdShow) 
{
    WNDCLASSEXA WindowClass = {};

    Win32LoadXInput();
    Win32ResizeDIBSection(&global_back_buffer, 1280, 720);

    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = instance;
    // WindowClass.hIcon = instance;
    WindowClass.lpszClassName = "NewGameWindowClass";
    // HICON     hIconSm;

    ATOM atom = RegisterClassExA(&WindowClass);
    
    if (atom) {
        HWND window = CreateWindowExA(0, WindowClass.lpszClassName, "Newgame", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

        if (window) {
            Running = true;
            global_back_buffer.x_offset = 0;
            global_back_buffer.y_offset = 0;

            // todo: develop loading schemes.
            // Load models into memory. 
            Obj* render_models;
            Obj* cube = LoadOBJToMemory("cube.obj");                 
            render_models = cube;

            while (Running == true) {
                MSG Message;
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT) {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                // Get device inputs.
                std::vector<XINPUT_GAMEPAD> inputs = GetDeviceInputs();
                ProcessInputs(&global_back_buffer, inputs);

                // Get keyboard inputs.
                // W.
                if (GetAsyncKeyState(87) >= 0) {
                   global_back_buffer.y_offset += -1;
                }

                // A.
                if (GetAsyncKeyState(65) >= 0) {
                    global_back_buffer.x_offset += 1;
                }

                // S.
                if (GetAsyncKeyState(83) >= 0) {
                    global_back_buffer.y_offset += 1;
                }

                // D.
                if (GetAsyncKeyState(68) >= 0) {
                    global_back_buffer.x_offset += -1;
                }

                RenderWeirdGradient(&global_back_buffer, global_back_buffer.x_offset, global_back_buffer.y_offset);

                // todo: render the cube!
                RenderModels(render_models);

                HDC device_context = GetDC(window);
                win32_window_dimensions client_rect = GetWindowDimension(window);

                Win32DisplayBufferInWindow(
                        &global_back_buffer, 
                        device_context, 0, 0, 
                        client_rect.width, 
                        client_rect.height);

                ReleaseDC(window, device_context);
            }
        }
        else {
            //TODO: Logging...
        }
    } 
    else {
        //TODO: Logging...
    }

    return(0);
}
