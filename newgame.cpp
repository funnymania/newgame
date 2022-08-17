#include <windows.h>

  LRESULT MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
  ) {
    LRESULT Result = 0;

    switch(Message) {
      case WM_SIZE: {
        OutputDebugStringA("WM_SIZE\n");
      } break;

      case WM_CLOSE: {
        OutputDebugStringA("WM_CLOSE\n");
      } break;

      case WM_DESTROY: {
        OutputDebugStringA("WM_DESTROY\n");
      } break;

      case WM_ACTIVATEAPP: {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
      } break;

      case WM_PAINT: {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);
        INT X = Paint.rcPaint.left;
        INT Y = Paint.rcPaint.top;
        LONG Width = Paint.rcPaint.right - Paint.rcPaint.left;
        LONG Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
        PatBlt(DeviceContext, X, Y, Width, Height, WHITENESS);
        EndPaint(Window, &Paint);
      } break;

      default: {
        OutputDebugStringA("default\n");
        Result = DefWindowProc(Window, Message, WParam, LParam);
      }break;
    }

    return(Result);
  }

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
  WNDCLASSEXA WindowClass = {};

  WindowClass.cbSize = sizeof(WNDCLASSEXA);
  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = hInstance;
  // WindowClass.hIcon = hInstance;
  WindowClass.lpszClassName = "NewGameWindowClass";
  // HICON     hIconSm;
  //

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
      for(;;) {
        MSG Message;
        BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
        if (MessageResult > 0) {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
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
