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

struct v3_float_64
{
    f64 x;
    f64 y;
    f64 z;
};

struct v3_float 
{
    f32 x;
    f32 y;
    f32 z;
};

struct Transform 
{
    v3 pos;
    v3 rot;
    // v3 scale;
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
    v3_float verts[3];
    v3_float normals[3];
};

struct Obj 
{
    Tri* triangles; // Vertice positions are OFFSETS from tra. A value of (1, 0, 0) is to the left of wherever tra is.
    Transform tra;
    u64 triangles_len;
};

struct Camera 
{
    v3 pos;
    v3 direction;    
    float wideness;
};

#endif
