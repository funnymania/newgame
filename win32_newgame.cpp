#include <windows.h>
#include <xinput.h>
#include <stdint.h>
#include <vector>

#include "geometry_m.h"
#include "newgame.h"
#include "allocator.cpp"
#include "win32_audio.cpp"

#include "newgame.cpp"

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

struct RunningModels 
{
    Obj* models;
    u64 models_len;
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
global_variable GameOffscreenBuffer buffer;

global_variable RunningModels runtime_models;

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


// note: callled on WM_SIZE.
internal void Win32ResizeDIBSection(GameOffscreenBuffer *buffer, LONG width, LONG height) 
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

internal void ProcessInputs(GameOffscreenBuffer *buffer, std::vector<XINPUT_GAMEPAD> inputs) 
{
    for (int i = 0; i < inputs.size(); i += 1) {
        if (inputs.at(i).sThumbLX > 5000) {
            buffer->x_offset += 2;
        } else if (inputs.at(i).sThumbLX < -5000) {
            buffer->x_offset += -2;
        } else  {
            // buffer->x_offset = 0;
        }

        if (inputs.at(i).sThumbLY > 5000) {
            buffer->y_offset += 2;
        } else if (inputs.at(i).sThumbLY< -5000) {
            buffer->y_offset += -2;
        } else {
            // buffer->y_offset = 0;
        }
    }
}

// note: callled on WM_PAINT.
internal void Win32DisplayBufferInWindow(GameOffscreenBuffer *buffer, 
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
            Win32ResizeDIBSection(&buffer, dimensions.width, dimensions.height);
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
            // OutputDebugStringA("WNDcallback\n");
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

            Win32DisplayBufferInWindow(&buffer, DeviceContext, X, Y, Width, height);
            EndPaint(Window, &Paint);
        } break;

        default: 
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

// note: assuming camera is at origin, with a direction vector positive in z, and wideness of 0.5.
bool IsTriangleInCamera(Tri* triangle, Camera camera, Transform tra)
{
    for (int counter = 0; counter < 3; counter += 1) {
        float distance = triangle->verts[counter].z + tra.pos.z - camera.pos.z;
        float frustrum_extent = distance * camera.wideness; 
        
        // Vertex is behind camera.
        if (distance < 0) {
            continue;
        }

        float cam_range_x = 1.78 / 2;
        float cam_range_y = 1 / 2;
        if (triangle->verts[counter].x + tra.pos.x < frustrum_extent * (camera.pos.x + cam_range_x)
            && triangle->verts[counter].x + tra.pos.x > frustrum_extent * (camera.pos.x - cam_range_x)
            && triangle->verts[counter].y + tra.pos.y < frustrum_extent * (camera.pos.y + cam_range_y)
            && triangle->verts[counter].y + tra.pos.y > frustrum_extent * (camera.pos.y - cam_range_y)
        ) {
           return(true); 
        }
    }

    return(false);
}

void RenderModels(RunningModels* models, Camera camera) 
{
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
    Obj* model_ptr = models->models;
    int counter = 0;
    while (counter < models->models_len) {
        Tri* triangle_ptr = model_ptr->triangles;
        int tri_counter = 0;
        while (tri_counter < model_ptr->triangles_len) {
            // From camera "center", take a vertex and test if, give it's Z-distance from camera, it is within
            // the rectangle that is the camera's "view" at that point!
            // there will be a formula such that at every 1 unit distance from camera, frustrum expands x and y
            // relative to aspect ratio. since our aspect ratio is 1280 by 720, this would mean... 
            // for every Z unit, frustrum increases 1.78y and 1x.
            if (IsTriangleInCamera(triangle_ptr, camera, model_ptr->tra)) {
                // given distance from camera, compute "size" and placement on the screen of tri
                // morph shape of tri given angle of camera and rotation of model containing tri. 
            }

            tri_counter += 1;
            triangle_ptr += 1;
        }

        counter += 1;
        model_ptr += 1;
    } 
    
    // calculate lighting to determine what each triangle should be colored as.
    
    // paint that imagery.
    // optionally send to GPU.
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR pCmdLine, int nCmdShow) 
{
    WNDCLASSEXA WindowClass = {};

    Win32LoadXInput();
    Win32ResizeDIBSection(&buffer, 1280, 720);

    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = instance;
    // WindowClass.hIcon = instance;
    WindowClass.lpszClassName = "NewGameWindowClass";
    // HICON     hIconSm;

    // Locked in value on OS boot, used for computing framerates.
    LARGE_INTEGER perf_counter_frequency_result;
    QueryPerformanceFrequency(&perf_counter_frequency_result);
    i64 perf_counter_frequency = perf_counter_frequency_result.QuadPart;

    ATOM atom = RegisterClassExA(&WindowClass);
    
    if (atom) {
        HWND window = CreateWindowExA(0, WindowClass.lpszClassName, "Newgame", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

        if (window) {
            HRESULT hr;
            Running = true;
            buffer.x_offset = 0;
            buffer.y_offset = 0;

            // todo: develop loading schemes.
            // Load models into memory. 
            Obj* cube = LoadOBJToMemory("cube.obj");                 
            runtime_models.models = cube;
            runtime_models.models_len += 1;

            // move the cube.
            cube->tra.pos = { 0, 0, 2 };

            Camera camera = { {0, 0, 0}, {0, 0, 1}, 0.5 };

            // 24/48.
            WAVEFORMATEX format_24_48 = {};
            format_24_48.wFormatTag = WAVE_FORMAT_PCM; 
            format_24_48.nChannels = 2; 
            format_24_48.nSamplesPerSec = 48000L; 
            format_24_48.nAvgBytesPerSec = 288000L; 
            format_24_48.nBlockAlign = 6; // channels * bits / 8
            format_24_48.wBitsPerSample = 24; 
            format_24_48.cbSize = 0;

            // 24/96.
            WAVEFORMATEX format_24_96 = {};
            format_24_96.wFormatTag = WAVE_FORMAT_PCM; 
            format_24_96.nChannels = 2; 
            format_24_96.nSamplesPerSec = 96000; 
            format_24_96.nAvgBytesPerSec = 576000L; 
            format_24_96.nBlockAlign = 6; // channels * bits / 8
            format_24_96.wBitsPerSample = 24; 
            format_24_96.cbSize = 0;

            // 16/44.
            WAVEFORMATEX format_16_44 = {};
            format_16_44.wFormatTag = WAVE_FORMAT_PCM; 
            format_16_44.nChannels = 2; 
            format_16_44.nSamplesPerSec = 44100; 
            format_16_44.nAvgBytesPerSec = 176400; format_16_44.nBlockAlign = 4; // channels * bits / 8
            format_16_44.nBlockAlign = 4; // channels * bits / 8
            format_16_44.wBitsPerSample = 16; 
            format_16_44.cbSize = 0;

            // Default spatial audio format (as concerns mcClure's Grado headphones at least).
            WAVEFORMATEX spatial_format = {};
            spatial_format.wFormatTag = 3; 
            spatial_format.nChannels = 1; 
            spatial_format.nSamplesPerSec = 48000L; 
            spatial_format.nAvgBytesPerSec = 192000L; 
            spatial_format.nBlockAlign = 4; // channels * bits / 8
            spatial_format.wBitsPerSample = 32; 
            spatial_format.cbSize = 0;

            // Start up Static and Spatial audio clients.
            InitStaticAudioClient(format_24_48); 
            InitStaticAudioClient(format_16_44); 
            InitStaticAudioClient(format_24_96); 
            ISpatialAudioClient* spatial_audio_client = InitSpatialAudioClient();

            // Determine if user has selected spatial sound.
            UINT32 create_spatial_streams;
            spatial_audio_client->GetMaxDynamicObjectCount(&create_spatial_streams);

            // Containers for playing sounds.
            // One SpatialAudioStream is sufficient for many spatial sounds.
            SpatialAudioStream spatial_audio;

            std::vector<AudioObj> spatial_sounds_objects;

            // note: this is entirely user decided via their Audio Settings. If they do not have Windows Sonic set for
            //       spatialized sound, this will always be 0. We cannot override that through code.
            if (create_spatial_streams == 0) {
                // StaticAudioStream stream = SetupStaticAudioStream("", format_24_48, static_client);
                // hr = stream.audio_client->Start();
            } else {
                spatial_audio = SetupSpatialAudioStream(spatial_format, spatial_audio_client, create_spatial_streams);

                hr = spatial_audio.render_stream->Start();

                PlaySpatialAudio(&spatial_audio, spatial_format, &spatial_sounds_objects);

                // Activate a new dynamic audio object. 
                // A stream can hold a max number of dynamic audio objects. 
                ISpatialAudioObject* audioObject;
                hr = spatial_audio.render_stream->ActivateSpatialAudioObject(AudioObjectType::AudioObjectType_Dynamic, 
                        &audioObject);

                // If SPTLAUDCLNT_E_NO_MORE_OBJECTS is returned, there are no more available objects
                if (SUCCEEDED(hr)) {
                    // Init new struct with the new audio object.
                    AudioObj obj = {
                        audioObject,
                        { -100, 0, 0 },
                        { 0.1, 0, 0 },
                        0.2,
                        3000, // Should be our song data...
                        spatial_format.nSamplesPerSec * 5 // 5 seconds of audio samples
                    };

                    spatial_sounds_objects.insert(spatial_sounds_objects.begin(), obj);
                }
            }

            LARGE_INTEGER last_counter;
            QueryPerformanceCounter(&last_counter);

            i64 last_cycle_count = __rdtsc();

            // need a few things
            // StartPlaying(audio)
            // StopAllAudio()
            // StopPlaying(audio)

            InitGame();
            // LoadSound("Eerie_Town.wav");
            // StartPlaying("Eerie_Town.wav");
            
            bool all_sounds_stop = false;

            // GAME LOOP.
            while (Running == true) {
                // Counter for determining time passed, with granularity greater than a microsecond.
                // This is relevant, as our game runs its update loop on the order of milliseconds.
                LARGE_INTEGER begin_counter;
                QueryPerformanceCounter(&begin_counter);

                MSG Message;
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT) {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                if (create_spatial_streams != 0) {
                    if (spatial_sounds_objects.size() > 0 && all_sounds_stop == false) {
                        PlaySpatialAudio(&spatial_audio, spatial_format, &spatial_sounds_objects);
                    } else {
                        // Stop the stream 
                        hr = spatial_audio.render_stream->Stop();
                    }
                }

                // Get device inputs.
                std::vector<XINPUT_GAMEPAD> inputs = GetDeviceInputs();
                ProcessInputs(&buffer, inputs);

                // Get keyboard inputs.
                // W.
                if (GetAsyncKeyState(87) & 0x01) {
                    buffer.y_offset += -2;
                    // Pause("Eerie_Town.wav");
                    // Reverse("Eerie_Town.wav");
                }

                // A.
                if (GetAsyncKeyState(65) & 0x01) {
                    buffer.x_offset += 2;
                    // Unpause("Eerie_Town.wav");
                }

                // S.
                if (GetAsyncKeyState(83) & 0x01) {
                    buffer.y_offset += 2;
                    // StopPlaying("Eerie_Town.wav");
                }

                // D.
                if (GetAsyncKeyState(68) & 0x01) {
                    buffer.x_offset += -2;
                }

                if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState(VK_F4)) {
                   Running = false;
                }

                // perf: This is our bottleneck!
                GameUpdateAndRender(&buffer);
                
                // todo: render the cube!
                RenderModels(&runtime_models, camera);

                HDC device_context = GetDC(window);
                win32_window_dimensions client_rect = GetWindowDimension(window);

                Win32DisplayBufferInWindow(
                        &buffer, 
                        device_context, 0, 0, 
                        client_rect.width, 
                        client_rect.height);

                ReleaseDC(window, device_context);

                i64 end_cycle_count = __rdtsc();
                
                LARGE_INTEGER end_counter;
                QueryPerformanceCounter(&end_counter);

                i64 cycles_elapsed = end_cycle_count - last_cycle_count;
                i64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
                i64 ms_per_frame = 1000 * counter_elapsed / perf_counter_frequency;
                i32 mz_per_frame = (i32)(cycles_elapsed / 1000 / 1000);

                char buffer_test[256];

#if 0
                i64 frames_per_second = 1000 / ms_per_frame;
                wsprintf(buffer_test, "Framerate: %dms - %dFPS - %d million cycles\n", ms_per_frame, 
                        frames_per_second, mz_per_frame);
                OutputDebugString(buffer_test);
#endif

                last_counter = end_counter;
                last_cycle_count = end_cycle_count;
            }
        }
        else {
            //TODO: Logging...
        }
    } 
    else {
        //TODO: Logging...
    }

    // // Reset the stream to free it's resources.
    // HRESULT hr_end = spatial_audio.render_stream->Reset();

    // CloseHandle(spatial_audio.buffer_completion_event);

    return(0);
}
