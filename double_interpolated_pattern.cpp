#include "double_interpolated_pattern.h"

DoubleInterpolatedPattern DoubleInterpolatedPattern::Create(Sequence_f32 sequence, Sequence_f32 sequence2) 
{
    DoubleInterpolatedPattern result = {};

    // init to -1.
    for (int i = 0; i < 60; i += 1) {
        result.value[i] = -1;
        result.value_2[i] = -1;
    }

    // Compute the full values.
    for (int i = 0; i < sequence.size - 1; i += 1) {
        f32 current_value = (&sequence.arr[2*i])[0];
        f32 next_value = (&sequence.arr[2 * (i + 1)])[0];

        int current_time = (int)(&sequence.arr[2*i])[1];
        int next_time = (int)(&sequence.arr[2 * (i + 1)])[1];
        
        for (int j = current_time; j < next_time; j += 1) {
            result.value[j] = 
                current_value + ((next_value - current_value) / (next_time - current_time) * (j - current_time));
        }

        result.value[next_time] = (&sequence.arr[2 * (i + 1)])[0];
    }

    for (int i = 0; i < sequence2.size - 1; i += 1) {
        f32 current_value = (&sequence2.arr[2*i])[0];
        f32 next_value = (&sequence2.arr[2*(i + 1)])[0];

        int current_time = (int)(&sequence2.arr[2*i])[1];
        int next_time = (int)(&sequence2.arr[2 * (i + 1)])[1];
        
        for (int j = current_time; j < next_time; j += 1) {
            result.value_2[j] = 
                current_value + ((next_value - current_value) / (next_time - current_time) * (j - current_time));
        }

        result.value_2[next_time] = (&sequence2.arr[2*(i + 1)])[0];
    }

    return(result);
}
