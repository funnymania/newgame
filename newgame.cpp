#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// TODO: Global to be removed later.
global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int bitmap_width;
global_variable int bitmap_height;
global_variable int bytes_per_pixel = 4;

internal void RenderWeirdGradient(int x_offset, int y_offset) {
    int width = bitmap_width;
    int height = bitmap_height;
    // BGRx
    int pitch = width * bytes_per_pixel;
    u8 *row = (u8 *)BitmapMemory; 
    for (int y = 0; y < bitmap_height; y += 1) {
        u8 *pixel = (u8 *)row;
        for (int x = 0; x < bitmap_width; x += 1) {
            *pixel = (u8)(x + x_offset);
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

internal void Win32ResizeDIBSection(LONG width, LONG height) {
    if (BitmapMemory) {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    bitmap_width = width;
    bitmap_height = height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = bitmap_width;
    BitmapInfo.bmiHeader.biHeight = -bitmap_height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmap_memory_size = bytes_per_pixel * bitmap_width * bitmap_height;
    BitmapMemory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

    RenderWeirdGradient(0, 0);
}

internal void Win32UpdateWindow(RECT *window_rect, HDC DeviceContext, int X, int Y, int Width, int height) {
    StretchDIBits(
        DeviceContext,
        0, 0, bitmap_width, bitmap_height,
        0, 0, Width, height,
        BitmapMemory,
        &BitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT Win32MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
) {
    LRESULT Result = 0;

    switch(Message) {
        case WM_SIZE: 
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            LONG width = ClientRect.right - ClientRect.left;
            LONG height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(width, height);
            // Win32UpdateWindow(&client_rect, DeviceContext, X, Y, Width, height);
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_CLOSE: 
        {
            Running = false;
        } break;

        case WM_DESTROY: 
        {
            Running = false;
        } break;

        case WM_ACTIVATEAPP: 
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT: 
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            
            INT X = Paint.rcPaint.left;
            INT Y = Paint.rcPaint.top;
            LONG Width = Paint.rcPaint.right - Paint.rcPaint.left;
            LONG height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            RECT client_rect;
            GetClientRect(Window, &client_rect);

            Win32UpdateWindow(&client_rect, DeviceContext, X, Y, Width, height);
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    WNDCLASSEXA WindowClass = {};

    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = hInstance;
    // WindowClass.hIcon = hInstance;
    WindowClass.lpszClassName = "NewGameWindowClass";
    // HICON     hIconSm;

    ATOM atom = RegisterClassExA(&WindowClass);
    
    if (atom) {
        HWND window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Newgame",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            hInstance,
            0
        );

        if (window) {
            int x_offset = 0;
            int y_offset = 0;
            Running = true;
            while (Running == true) {
                MSG Message;
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT) {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                RenderWeirdGradient(x_offset, y_offset);

                HDC device_context = GetDC(window);
                RECT client_rect;
                GetClientRect(window, &client_rect);

                int window_width = client_rect.right - client_rect.left;
                int window_height = client_rect.bottom - client_rect.top;

                Win32UpdateWindow(&client_rect, device_context, 0, 0, window_width, window_height);
                ReleaseDC(window, device_context);

                x_offset += 1;
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
