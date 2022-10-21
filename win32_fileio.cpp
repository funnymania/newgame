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

void PlatformWriteEntireFile(char* file_name, u32 data_len, void* data)
{
    // Get a "file pointer"
    HANDLE file_handle = CreateFileA(file_name, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    if (file_handle != INVALID_HANDLE_VALUE) {
        // Write some bytes to it.
        DWORD bytes_written;
        WriteFile(file_handle, data, data_len, &bytes_written, 0);
        CloseHandle(file_handle);
    } else {
        OutputDebugString("Writing to file failed!");
    }
}

enum class FileWriteOptions
{
    NONE,
    TIMESTAMP
};

u32 PlatformGenerateTimestamp(GameMemory* game_memory, char* timestamp_debug) 
{
    SYSTEMTIME local_time;
    GetLocalTime(&local_time);

    timestamp_debug = (char*)game_memory->next_available;

    int tens_place = local_time.wHour % 10;
    timestamp_debug[0] = digit_char_convert[tens_place];
    int ones_place = local_time.wHour - tens_place * 10;
    timestamp_debug[1] = digit_char_convert[ones_place];
    timestamp_debug[2] = ':';

    tens_place = local_time.wMinute % 60;
    timestamp_debug[3] = digit_char_convert[tens_place];
    ones_place = local_time.wMinute - tens_place * 10;
    timestamp_debug[4] = digit_char_convert[ones_place];
    timestamp_debug[5] = ':';

    tens_place = local_time.wSecond % 60;
    timestamp_debug[6] = digit_char_convert[tens_place];
    ones_place = local_time.wSecond - tens_place * 10;
    timestamp_debug[7] = digit_char_convert[ones_place];

    game_memory->next_available += 8;
    game_memory->permanent_storage_remaining -= 8;

    return(8);
}

// todo: append to file instead.
void PlatformWriteLineToFile(char* file_name, u32 data_len, void* data, FileWriteOptions options, 
        GameMemory* game_memory)
{
    // Get a "file pointer"
    HANDLE file_handle = CreateFileA(file_name, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    if (file_handle != INVALID_HANDLE_VALUE) {
        char* debug_string = 0; 
        char* seek = debug_string;
        char* time_stamp = 0;

        DWORD total_line_length = 0;

        if (options == FileWriteOptions::TIMESTAMP) { 
            u32 list_len = PlatformGenerateTimestamp(game_memory, debug_string);
            seek = debug_string + list_len;
            total_line_length += list_len;
        } else {
            debug_string = (char*)game_memory->next_available;
            seek = debug_string;
        }

        memcpy(seek, data, data_len);

        total_line_length += data_len;
        game_memory->next_available += data_len;
        game_memory->permanent_storage_remaining -= data_len;

        DWORD bytes_written;
        WriteFile(file_handle, debug_string, total_line_length, &bytes_written, 0);
        CloseHandle(file_handle);

        // clear memory.
        ZeroByOffset(1, total_line_length, game_memory);
    } else {
        OutputDebugString("Writing to file failed!");
    }
}
