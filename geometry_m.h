#ifndef GEOMETRY_M_H
#define GEOMETRY_M_H

struct Sausage 
{
    u64 radius;
    u64 length;
    Transform tra;
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
