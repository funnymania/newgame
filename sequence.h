#ifndef SEQUENCE_H
#define SEQUENCE_H

// List of 2-arrays
struct Sequence_f32 
{
    f64* arr;
    u64 size;

    Sequence_f32() {}
    static Sequence_f32 Create(f32 array[][2], int arr_size, GameMemory* game_memory);
};

#endif
