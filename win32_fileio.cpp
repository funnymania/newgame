// Asks for memory appropriately circumstantially.
#include <stdio.h>
#include <string>

static DWORD bytes_read;

void PlatformFreeFileMemory (void* memory) 
{
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

// todo: Second VirtualAlloc occurs here, we need to pull from game_memory->next_available.
void PlatformReadEntireFile(char* file_name, FileReadResult* file_result) 
{
    HANDLE file_handle = CreateFileA(file_name, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

    file_result->data = 0;

    if (file_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER file_size;
        if (GetFileSizeEx(file_handle, &file_size)) {
            // Assert(file_size.QuadPart <= 0xFFFFFFFF);
            u32 file_size_32 = (u32)file_size.QuadPart;
            file_result->data = VirtualAlloc(0, file_size_32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE); 
            if (file_result->data) {
                if (ReadFile(file_handle, file_result->data, file_size_32, &bytes_read, 0) 
                        && file_size_32 == bytes_read) {
                    file_result->data_len = file_size_32;
                } else {
                    PlatformFreeFileMemory(file_result->data);
                }
            } else {
                //todo: logs.
            }
        } else {
            //todo: logs.
        }

        CloseHandle(file_handle);
    }
}
