#include <vector>

#ifndef GEOMETRY_M_H
#define GEOMETRY_M_H

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

struct v3 
{
    u64 x;
    u64 y;
    u64 z;
};

struct v3_float 
{
    f64 x;
    f64 y;
    f64 z;
};

struct Transform 
{
    v3 pos;
    v3 rot;
    v3 scale;
};

struct Ray 
{
    v3 begin;
    v3 end;
};

struct Sausage 
{
    u64 radius;
    u64 length;
    Transform tra;
};

struct Tri 
{
    v3_float one;
    v3_float two;
    v3_float three;
    v3_float one_normal;
    v3_float two_normal;
    v3_float three_normal;
};

struct Obj 
{
    Tri* triangles;
    u64 triangles_len;
};

#endif
