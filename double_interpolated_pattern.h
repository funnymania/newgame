#ifndef DOUBLE_INTERPOLATED_PATTERN_H
#define DOUBLE_INTERPOLATED_PATTERN_H

// todo: value and value_2 must be lists.
struct DoubleInterpolatedPattern
{
    f32 value[60];    // where '60' is frames (aka time)
    f32 value_2[60];  // where '60' is frames (aka time)
    u16 current_time; // starts at 0, ends at '60'
    u16 length;
                      
    DoubleInterpolatedPattern() {}

    // note: sequence must always specific a starting and ending value.
    static DoubleInterpolatedPattern Create(Sequence_f32 sequence, Sequence_f32 sequence2);
};

typedef void rumble_controller(u32 device_index, DoubleInterpolatedPattern* pattern);

#endif
