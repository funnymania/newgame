#include "newgame.h"
#include "sequence.h"

Sequence_f32 Sequence_f32::Create(f32 array[][2], int arr_size, GameMemory* game_memory) 
{
    Sequence_f32 result = {};
    result.arr = (f32*)game_memory->next_available;

    f32* seek = result.arr;
    // init arr for arr_size.
    for (int counter = 0; counter < arr_size; counter += 1) {
        for (int counter2 = 0; counter2 < 2; counter2 += 1) {
            *seek = array[counter][counter2];
            game_memory->permanent_storage_remaining -= 4;
            game_memory->next_available += 4;
            seek += 1;
        }
    }

    result.size = arr_size;
    return(result);
}
