#ifndef GEOMETRY_M_H
#define GEOMETRY_M_H

// todo: derive this ourselves!
#define PI 3.14159265358979323846

enum MorphStatus 
{
    NO_MORPH,
    SPHERE,
};

// perf: triangles should probably be storing pointers to 'vertices', since changing the vertices involves also
//       changing the triangles everytime, and if we were to support removing vertices, this would require 
//       re-triangulating the polyhedron anyways, which would re-generate triangles and map the vertices onto that.
struct Polyhedron
{
    Transform transform;
    MorphStatus morph_status;
    Color color;
    SimpList<v3f64> vertices;
    SimpList<TriangleRefVertices> triangles;
};

struct Sausage 
{
    u64 radius;
    u64 length;
    Transform tra;
};

#endif
