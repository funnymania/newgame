// Moves next_available back or forwards some `offset` amount, zeroing any memory in the offset.
void ZeroByOffset(u64 size, u32 offset, GameMemory* game_memory) 
{
    u64 bytes = size * offset;
    u8* seek = (u8*)game_memory->next_available;
    if (bytes < 0) {
        for (i32 a = 0; a > bytes; a -= 1) {
            *seek = 0;
            seek -= 1;
        }
    } else {
        for (i32 a = 0; a < bytes; a += 1) {
            *seek = 0;
            seek += 1;
        }
    }

    game_memory->next_available += bytes;
    game_memory->permanent_storage_remaining -= bytes;
}

void ZeroMemoryNonWindows(void* obj_ptr, u64 obj_count, u32 obj_size) 
{
    u8* tmp = (u8*)obj_ptr;
    for (int counter = 0; counter < obj_size * obj_count; counter += 1) 
    {
        *tmp = 0;
        tmp += 1;
    }
}

#define ZeroMemory ZeroMemoryNonWindows
