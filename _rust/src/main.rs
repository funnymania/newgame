use windows::{
    core::*,
    Win32::Foundation::*,
    Win32::Graphics::Gdi::*, 
    Win32::System::LibraryLoader::GetModuleHandleA,
    Win32::UI::WindowsAndMessaging::*
};

use std::mem::size_of;
use core::ffi::c_void;

//TODO: Maybe think about having this GLOBAL STATE.
static mut Running: bool = true;
static mut BitmapInfo: BITMAPINFO = BITMAPINFO {
    bmiHeader: BITMAPINFOHEADER {
        biSize: 0,
        biWidth: 0,
        biHeight: 0,
        biPlanes: 0,
        biBitCount: 0,
        biCompression: 0,
        biSizeImage: 0,
        biXPelsPerMeter: 0,
        biYPelsPerMeter: 0,
        biClrUsed: 0,
        biClrImportant: 0,
    },
    bmiColors: [RGBQUAD { rgbBlue: 0, rgbGreen: 0, rgbRed: 0, rgbReserved: 0 }],
};
static mut BitmapMemory: *mut c_void = std::ptr::null_mut();
static mut BitmapHandle: HBITMAP = HBITMAP(0);
static mut BitmapDeviceContext: CreatedHDC = CreatedHDC(0);

fn main() -> Result<()> {
    //TODO: What can maybe not be in unsafe block?
    unsafe {
        let instance = GetModuleHandleA(None)?;

        let mut WindowClass = WNDCLASSEXA::default();
        WindowClass.cbSize = size_of::<WNDCLASSEXA>() as u32;
        WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
        WindowClass.lpfnWndProc = Some(Win32MainWindowCallback);
        WindowClass.hInstance = instance;
        // WindowClass.hIcon = Instance;
        WindowClass.lpszClassName = PCSTR(b"NewGameWindowClass\0".as_ptr());
        // HICON     hIconSm;

        let atom = RegisterClassExA(&WindowClass);

        if atom != 0 {
            let WindowHandle = CreateWindowExA(
                WINDOW_EX_STYLE(0),
                WindowClass.lpszClassName,
                PCSTR(b"Newgame\0".as_ptr()),
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                HWND(0),
                HMENU(0),
                instance,
                std::ptr::null(),
            );

            if WindowHandle.0 != 0 {
                Running = true;
                while Running == true {
                    let mut Message = MSG::default();

                    // note: Message is being COERCED into a mutable pointer, BECAUSE the function 
                    //       GetMessageA takes a mutable pointer, and that's just what Rust does
                    //       when you pass a mutable reference to a function like this.
                    let MessageResult = GetMessageA(&mut Message, HWND(0), 0, 0);
                    if MessageResult.0 > 0 {
                        TranslateMessage(&Message);
                        DispatchMessageA(&Message);
                    }
                    else {
                      break;
                    }
              }
            } else {
              //TODO: Logging...
            }
        } else {
            //TODO: Logs.
        }
    }

    Ok(())
}

extern "system" fn Win32UpdateWindow(device_context: HDC, window: HWND, x: i32, y: i32, width: i32, height: i32) {
    unsafe {
        StretchDIBits(
            device_context,
            x,
            y,
            width,
            height,
            x, y, width, height,
            BitmapMemory,
            &BitmapInfo,
            DIB_RGB_COLORS,
            SRCCOPY
        );
    }
}

extern "system" fn Win32MainWindowCallback(window: HWND, message: u32, wparam: WPARAM, lparam: LPARAM) -> LRESULT {
    unsafe {
        match message {
            WM_SIZE => {
                let mut client_rect = RECT::default();
                GetClientRect(window, &mut client_rect);
                let width: i64 = (client_rect.right - client_rect.left) as i64;
                let height: i64 = (client_rect.bottom - client_rect.top) as i64;
                Win32ResizeDIBSection(width, height);
                println!("WM_SIZE");
                LRESULT(0)
            }
            // perf: RAM increases here as Window increases in size, and vice versa.
            WM_PAINT => {
              let mut Paint = PAINTSTRUCT::default();
              let DeviceContext = BeginPaint(window, &mut Paint);
              
              let X = Paint.rcPaint.left;
              let Y = Paint.rcPaint.top;
              let Width = Paint.rcPaint.right - Paint.rcPaint.left;
              let Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
              Win32UpdateWindow(DeviceContext, window, X, Y, Width, Height);

              PatBlt(DeviceContext, X, Y, Width, Height, WHITENESS);
              EndPaint(window, &mut Paint);
                LRESULT(0) // This is a tuple struct, so you can do this!
            }
            WM_DESTROY => {
                println!("WM_DESTROY");
                PostQuitMessage(0);
                LRESULT(0)
            }
            _ => DefWindowProcA(window, message, wparam, lparam),
        }
    }
}

extern "system" fn Win32ResizeDIBSection(width: i64, height:i64) {
    unsafe {
        if BitmapHandle.0 != 0 {
            DeleteObject(BitmapHandle);
        } 

  if BitmapDeviceContext.0 == 0 {
    //TODO: Recreate these under certain circumstances??
    BitmapDeviceContext = CreateCompatibleDC(HDC(0));
  }

  BitmapInfo.bmiHeader.biSize = size_of::<BITMAPINFOHEADER>() as u32;
  BitmapInfo.bmiHeader.biWidth = width as i32;
  BitmapInfo.bmiHeader.biHeight = height as i32;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB as u32;

  BitmapHandle = CreateDIBSection(
    BitmapDeviceContext,
    &BitmapInfo,
    DIB_RGB_COLORS,
    &mut BitmapMemory,
    HANDLE(0),
    0
  ).unwrap();
    }
}
