use windows::{
    core::*,
    Win32::Foundation::*,
    Win32::UI::WindowsAndMessaging::*
};

fn main() {
    unsafe {
        MessageBoxA(
            None, 
            PCSTR::from_raw(b"This is newgame.\0".as_ptr()),
            PCSTR::from_raw(b"New Game\0".as_ptr()), 
            MB_OK | MB_ICONINFORMATION);
    }
}
