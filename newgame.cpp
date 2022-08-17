#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

// TODO: Global to be removed later.
global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

internal void Win32ResizeDIBSection(LONG Width, LONG Height) {
  //TODO: Maybe don't free first, free later.
  if (BitmapHandle) {
    DeleteObject(BitmapHandle);
  } 

  if (BitmapDeviceContext == false) {
    //TODO: Recreate these under certain circumstances??
    BitmapDeviceContext = CreateCompatibleDC(0);
  }

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biHeight = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  BitmapHandle = CreateDIBSection(
    BitmapDeviceContext,
    &BitmapInfo,
    DIB_RGB_COLORS,
    &BitmapMemory,
    0,
    0
  );
}

internal void Win32UpdateWindow(HDC DeviceContext, HWND Window, int X, int Y, int Width, int Height) {
  StretchDIBits(
    DeviceContext,
    X,
    Y,
    Width,
    Height,
    X, Y, Width, Height,
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
      LONG Width = ClientRect.right - ClientRect.left;
      LONG Height = ClientRect.bottom - ClientRect.top;
      Win32ResizeDIBSection(Width, Height);
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
      LONG Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      Win32UpdateWindow(DeviceContext, Window, X, Y, Width, Height);

      PatBlt(DeviceContext, X, Y, Width, Height, WHITENESS);
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
    HWND WindowHandle = CreateWindowExA(
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

    if (WindowHandle) {
      Running = true;
      while (Running == true) {
        MSG Message;
        BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
        if (MessageResult > 0) {
          TranslateMessage(&Message);
          DispatchMessageA(&Message);
        }
        else {
          break;
        }
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
