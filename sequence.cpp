#include "sequence.h"

Sequence_f32 Sequence_f32::Create(f32 array[][2], int arr_size, GameMemory* game_memory) 
{
    Sequence_f32 result = {};
    result.arr = (f64*)game_memory->next_available;

    f64* seek = result.arr;
    // init arr for arr_size.
    for (int counter = 0; counter < arr_size; counter += 1) {
        for (int counter2 = 0; counter2 < 2; counter2 += 1) {
            *seek = (f64)array[counter][counter2];
            game_memory->permanent_storage_remaining -= 8;
            game_memory->next_available += 8;
            seek += 1;
        }
    }

    result.size = arr_size;
    return(result);
}
