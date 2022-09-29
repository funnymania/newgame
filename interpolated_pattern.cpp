#include "interpolated_pattern.h"

InterpolatedPattern InterpolatedPattern::Create(SimpList<v4i64> in_pattern, GameMemory* game_memory) 
{
    InterpolatedPattern result = {};
    result.current_time = 0;
    result.values.array = (v4i64*)game_memory->next_available;

    v4i64* seek = in_pattern.array;

    seek += in_pattern.length - 1;
    result.length = (u16)((*seek).t + 1);

    // Initialize list.
    v4i64* seek_result = result.values.array;
    seek_result += 1;
    for (i32 i = 1; i < result.length; i += 1) {
        *seek_result = {};
        seek_result += 1;
        game_memory->next_available += sizeof(v4i64);
        game_memory->permanent_storage_remaining -= sizeof(v4i64);
    }

    // bug: vales seem to be off by one, so we are just moving up by one to account.
    //      this is more a 'hack' than a bug, but we think of hacks as bugs.
    game_memory->next_available += sizeof(v4i64);
    game_memory->permanent_storage_remaining -= sizeof(v4i64);

    seek = in_pattern.array;
    seek_result = result.values.array;
    for (i64 i = 0; i < in_pattern.length - 1; i += 1) {
        // Compute the full values.
        v4i64 current_value = *seek;
        v4i64 next_value = *(seek + 1);
        
        for (i64 j = current_value.t; j < next_value.t; j += 1) {
            *seek_result = {
                current_value.x + ((next_value.x - current_value.x) / (next_value.t - current_value.t)) 
                    * (j - current_value.t),
                current_value.y + ((next_value.y - current_value.y) / (next_value.t - current_value.t)) 
                    * (j - current_value.t),
                current_value.z + ((next_value.z - current_value.z) / (next_value.t - current_value.t)) 
                    * (j - current_value.t)
            };

            seek_result += 1;
        }

        *seek_result = next_value;
        seek += 1;
    }

    return(result);
}
