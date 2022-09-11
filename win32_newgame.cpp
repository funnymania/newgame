#include <windows.h>
#include <xinput.h>
#include <stdint.h>
#include <vector>
#include <timeapi.h>

#include "primitives.h"
#include "list.cpp"
#include "sequence.cpp"
#include "double_interpolated_pattern.cpp"
#include "geometry_m.h"
#include "win32_fileio.cpp"
#include "win32_gamepad.cpp"
#include "win32_audio.cpp"
#include "platform_services.h"

#include "asset_table.cpp"

struct Win32OffscreenBuffer
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

struct Win32GameLib 
{
    HMODULE dll_handle;
    FILETIME last_write_time;
};

// note: this is a function TYPE. game_update_loop is now a TYPE of function that we can reference.
typedef void game_update_loop(GameOffscreenBuffer* buffer, std::vector<AngelInput> inputs, 
        GameMemory* platform_memory, PlatformServices services, bool continue_playing);

// note: basically a 'zero-initialized' function. We need to initialize our pointer below this line to SOMETHING. 
//       This is that something.
void GameUpdateLoopStub(GameOffscreenBuffer* buffer, std::vector<AngelInput> inputs, GameMemory* platform_memory,
       PlatformServices services, bool continue_playing) {}

global_variable game_update_loop* GameUpdateAndRender_ = GameUpdateLoopStub;
#define GameUpdateAndRender GameUpdateAndRender_

inline FILETIME Win32GetLastWrite(char* file_name) 
{
    FILETIME last_write = {};

    WIN32_FIND_DATA find_data;
    HANDLE find_handle = FindFirstFileA(file_name, &find_data);
    if (find_handle != INVALID_HANDLE_VALUE) {
        last_write = find_data.ftLastWriteTime;
        FindClose(find_handle);
    }

    return(last_write);
}

internal Win32GameLib Win32LoadGameDLL(char* file_name) 
{
    Win32GameLib game_lib = {};

    char* temp_dll = "../build/newgame_tmp.dll";

    game_lib.last_write_time = Win32GetLastWrite(file_name);

    CopyFileA(file_name, temp_dll, FALSE);
    game_lib.dll_handle = LoadLibrary("newgame_tmp.dll");
    if (game_lib.dll_handle) {
       GameUpdateAndRender = (game_update_loop*)GetProcAddress(game_lib.dll_handle, "GameUpdateAndRender");
    }

    return(game_lib);
}

internal void Win32UnloadGameLib(Win32GameLib lib)
{
    if (lib.dll_handle) {
        FreeLibrary(lib.dll_handle);
    }

    GameUpdateAndRender = GameUpdateLoopStub;
}

global_variable Win32OffscreenBuffer win32_buffer;
global_variable bool program_running = true;

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
internal void Win32ResizeDIBSection(Win32OffscreenBuffer *screen_buffer, LONG width, LONG height) 
{
    if (screen_buffer->memory) {
        VirtualFree(screen_buffer->memory, 0, MEM_RELEASE);
    }

    screen_buffer->width = width;
    screen_buffer->height = height;

    screen_buffer->info.bmiHeader.biSize = sizeof(screen_buffer->info.bmiHeader);
    screen_buffer->info.bmiHeader.biWidth = screen_buffer->width;
    screen_buffer->info.bmiHeader.biHeight = -screen_buffer->height;
    screen_buffer->info.bmiHeader.biPlanes = 1;
    screen_buffer->info.bmiHeader.biBitCount = 32;
    screen_buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmap_memory_size = screen_buffer->bytes_per_pixel * screen_buffer->width * screen_buffer->height;
    screen_buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32DisplayBufferInWindow(Win32OffscreenBuffer *screen_buffer, 
        HDC DeviceContext, int X, int Y, int width, int height) 
{
    // note: width and height are the dimensions of the rectangle to paint to.
    //       screen_buffer->width is the width of the rectangle to paint from.
    StretchDIBits(DeviceContext, 0, 0, screen_buffer->width, screen_buffer->height, 0, 0, screen_buffer->width, 
            screen_buffer->height, 
            screen_buffer->memory, &screen_buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT Win32MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
) {
    LRESULT Result = 0;

    //note: Not rendering when User is moving window
    switch(Message) {
        case WM_SIZE: 
        {
            // study: on resize, game loop is not running, probably because of looping on received messages, 
            //        a stream of WM_SIZE?
            win32_window_dimensions dimensions = GetWindowDimension(Window);
            Win32ResizeDIBSection(&win32_buffer, dimensions.width, dimensions.height);
        } break;

        case WM_DESTROY: 
        {
            // study: how to kill the loop in WinMain without a global?
            program_running = false;
        } break;

        case WM_QUIT: 
        {
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
        } break;

        case WM_KEYUP:
        {
        } break;

        case WM_PAINT: 
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            
            INT X = Paint.rcPaint.left;
            INT Y = Paint.rcPaint.top;
            LONG Width = Paint.rcPaint.right - Paint.rcPaint.left;
            LONG height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            Win32DisplayBufferInWindow(&win32_buffer, DeviceContext, X, Y, Width, height);
            EndPaint(Window, &Paint);
        } break;

        default: 
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

inline LARGE_INTEGER Win32GetWallclock() 
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR pCmdLine, int nCmdShow) 
{
    WNDCLASSEXA WindowClass = {};

    // Set sleep time to be settable down to the millisecond.
    u8 scheduler_ms = 1;
    bool granularized_sleep = (timeBeginPeriod(scheduler_ms) == TIMERR_NOERROR);

    Win32LoadXInput();

    char* source_dll = "../build/newgame.dll";
    Win32GameLib game_dll_result = Win32LoadGameDLL(source_dll);

    Win32ResizeDIBSection(&win32_buffer, 1280, 720);

    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = instance;
    WindowClass.lpszClassName = "NewGameWindowClass";

    // Application icons.
    // study: we are using some stand-in .ico file that windows seems to understand, but we don't know why.
    //       What hasn't worked:  -BMP files saved in paint (256 color and 16 bit color, 32 pixels and 48).
    //                            -Trying to save as ICO in Photoshop (could not get plugin to work)
    //                            -Renaming BMP and JPG file extensions to ICO.
    HICON ico = (HICON)LoadImageA(0, "app_icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    HICON ico_small = (HICON)LoadImageA(0, "app_icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    WindowClass.hIcon = ico;
    WindowClass.hIconSm = ico_small;
    
    // Locked in value on OS boot, used for computing framerates.
    LARGE_INTEGER perf_counter_frequency_result;
    QueryPerformanceFrequency(&perf_counter_frequency_result);
    i64 perf_counter_frequency = perf_counter_frequency_result.QuadPart;

    if (RegisterClassExA(&WindowClass)) {
        HWND window = CreateWindowExA(0, WindowClass.lpszClassName, "Newgame", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

        if (window) {
            HRESULT hr;
            HDC refresh_DC = GetDC(window);

            // Get monitor's refresh rate.
            int monitor_refresh_rate = 60;
            int win32_refresh_rate = GetDeviceCaps(refresh_DC, VREFRESH);
            if (win32_refresh_rate > 1) {
                monitor_refresh_rate = win32_refresh_rate;
            }

            f32 game_update_rate = (monitor_refresh_rate / 2.0f);
            f32 target_seconds_per_frame = 1.0f / game_update_rate;

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
            if (create_spatial_streams != 0) {
                // StaticAudioStream stream = SetupStaticAudioStream("", format_24_48, static_client);
                // hr = stream.audio_client->Start();
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
                        { 0.1f, 0, 0 },
                        0.2f,
                        3000, // Should be our song data...
                        spatial_format.nSamplesPerSec * 5 // 5 seconds of audio samples
                    };

                    spatial_sounds_objects.insert(spatial_sounds_objects.begin(), obj);
                }
            } 

            // Counter for determining time passed.
            LARGE_INTEGER last_counter;
            QueryPerformanceCounter(&last_counter);

            i64 last_cycle_count = __rdtsc();

#if NEWGAME_INTERNAL
            LPVOID base_address = (LPVOID)Terabytes((u64)2);
#else
            LPVOID base_address = 0;
#endif
            GameMemory game_memory = {};
            game_memory.permanent_storage_size = Megabytes(64);
            game_memory.transient_storage_size = Gigabytes((u64)4);

            u64 total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;

            game_memory.permanent_storage = VirtualAlloc(base_address, total_size, MEM_COMMIT|MEM_RESERVE,
                    PAGE_READWRITE);
            game_memory.transient_storage = (u8*)game_memory.permanent_storage + game_memory.permanent_storage_size;

            game_memory.permanent_storage_remaining = game_memory.permanent_storage_size;
            game_memory.transient_storage_remaining = game_memory.transient_storage_size;
            game_memory.next_available = (u8*)game_memory.permanent_storage;

            Assert(game_memory.permanent_storage_size >= Megabytes(64));

            // Services the Platform is providing to the Game.
            PlatformServices services = {};
            services.load_obj = LoadOBJToMemory;
            services.persona_handshake = Persona4Handshake;
            services.load_sound = LoadSound;
            services.start_playing = StartPlaying;
            services.pause_audio = Pause;
            services.resume_audio = Unpause;

            std::vector<AngelInput> inputs;
            Win32InitInputArray(&inputs);
            Win32InitKeycodeMapping(&game_memory);

            int counter = 0;
            bool continue_playing = true;

            while (program_running == true) {
                FILETIME new_write_time = Win32GetLastWrite(source_dll);
                if (CompareFileTime(&new_write_time, &game_dll_result.last_write_time) != 0) {
                    // Reload game code if the DLL has been modified.
                    continue_playing = false;
                    Win32UnloadGameLib(game_dll_result);
                    source_dll = "../build/newgame.dll";
                    game_dll_result = Win32LoadGameDLL(source_dll);
                }
                
                MSG Message;
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT) {
                        program_running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                if (create_spatial_streams != 0) {
                    if (spatial_sounds_objects.size() > 0) {
                        PlaySpatialAudio(&spatial_audio, spatial_format, &spatial_sounds_objects);
                    } else {
                        // Stop the stream 
                        hr = spatial_audio.render_stream->Stop();
                    }
                }

                // Get device inputs.
                GetDeviceInputs(&inputs, &program_running);

                GameOffscreenBuffer screen_buffer = {};
                screen_buffer.memory = win32_buffer.memory;
                screen_buffer.width = win32_buffer.width;
                screen_buffer.height = win32_buffer.height;
                screen_buffer.bytes_per_pixel = win32_buffer.bytes_per_pixel;

                // Call on game to provide joy.
                GameUpdateAndRender(&screen_buffer, inputs, &game_memory, services, continue_playing);

                HDC device_context = GetDC(window);
                win32_window_dimensions client_rect = GetWindowDimension(window);

                Win32DisplayBufferInWindow(&win32_buffer, device_context, 0, 0, client_rect.width, client_rect.height);
                ReleaseDC(window, device_context);

                i64 end_cycle_count = __rdtsc();
                
                LARGE_INTEGER end_counter = Win32GetWallclock();

                i64 cycles_elapsed = end_cycle_count - last_cycle_count;
                i64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;

                f32 seconds_elapsed_per_work = (f32)counter_elapsed / (f32)perf_counter_frequency;
                f32 seconds_elapsed_per_frame = seconds_elapsed_per_work;
                if (seconds_elapsed_per_frame < target_seconds_per_frame) {
                    while (seconds_elapsed_per_frame < target_seconds_per_frame) {
                        if (granularized_sleep) {
                            DWORD ms_to_sleep = (DWORD)(1000 * (target_seconds_per_frame - seconds_elapsed_per_frame));
                            Sleep(ms_to_sleep);
                        }

                        LARGE_INTEGER end_count;
                        QueryPerformanceCounter(&end_count);
                        seconds_elapsed_per_frame = (f32)(end_count.QuadPart - last_counter.QuadPart) 
                            / (f32)perf_counter_frequency;
                        // timeEndPeriod(ms_to_sleep);
                    }
                } else {
                    // note: frame rate miss... 
                }

                end_counter = Win32GetWallclock();
                counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;

                i64 ms_per_frame = 1000 * counter_elapsed / perf_counter_frequency;
                i32 mz_per_frame = (i32)(cycles_elapsed / 1000 / 1000);

                char buffer_test[256];

                i64 frames_per_second = 1000 / ms_per_frame;
                wsprintf(buffer_test, "Framerate: %dms - %dFPS - %d million cycles\n", ms_per_frame, 
                        frames_per_second, mz_per_frame);
                // note: uncomment for some playtime metrics. Currently conflicting with some debugging.
                OutputDebugString(buffer_test);

                last_counter = end_counter;
                last_cycle_count = end_cycle_count;
            }
        }
        else {
            //TODO: Logging...
        }
    } 
    else {
        // todo: Log files.
        DWORD error = GetLastError();
        OutputDebugString("Class Registration failed");
    }

    // // Reset the stream to free it's resources.
    // HRESULT hr_end = spatial_audio.render_stream->Reset();

    // CloseHandle(spatial_audio.buffer_completion_event);

    return(0);
}
