#ifndef DOUBLE_INTERPOLATED_PATTERN_H
#define DOUBLE_INTERPOLATED_PATTERN_H

struct DoubleInterpolatedPattern
{
    f32 value[60];    // where '60' is frames (aka time)
    f32 value_2[60];  // where '60' is frames (aka time)
    u16 current_time; // starts at 0, ends at '60'
                      
    DoubleInterpolatedPattern() {}

    // note: sequence must always specific a starting and ending value.
    static DoubleInterpolatedPattern Create(Sequence_f32 sequence, Sequence_f32 sequence2);
};

#endif
