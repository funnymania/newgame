#ifndef INTERPOLATED_PATTERN_H
#define INTERPOLATED_PATTERN_H

// note: frame-by-frame interpolation, every time-slice is a frame long. This is not a fixed duration. 
//       A frame is approx half the monitor refresh rate per second. Could be anywhere from 30 FPS to even 200. 
//       Developer's monitor is 144 frames per second.
struct InterpolatedPattern
{
    List<v4i64> values;
    u16 current_time;
    u16 length;
                      
    InterpolatedPattern() {}

    // note: sequence must always specific a starting and ending value.
    static InterpolatedPattern Create(List<v4i64> values, GameMemory* game_memory);
};

#endif
