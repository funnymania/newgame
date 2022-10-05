#include <stdint.h>
#include <vector>
#include <math.h>

#include "primitives.h"
#include "list.cpp"
#include "sequence.cpp"
#include "double_interpolated_pattern.cpp"
#include "interpolated_pattern.cpp"
#include "game_state.cpp"
#include "geometry_m.h"
#include "area.cpp"
#include "newgame.h"
#include "platform_services.h"

static u32 running_sounds_len = 0;

static SoundAssetTable sound_assets = {};

static Input final_inputs[MAX_DEVICES];
static AngelInputArray angel_inputs = {};

// Object for dual-motor rumbling.
static DoubleInterpolatedPattern rumble_patterns[MAX_DEVICES] = {};

static Area current_area;

static i64 view_move_x = 0;
static i64 view_move_y = 0;
static i64 view_move_z = 0;
static i64 auto_save_timer;

void ZeroMemory(void* obj_ptr, u64 obj_count, u32 obj_size) 
{
    u8* tmp = (u8*)obj_ptr;
    for (int counter = 0; counter < obj_size * obj_count; counter += 1) 
    {
        *tmp = 0;
        tmp += 1;
    }
}

// Moves next_available back or forwards some `offset` amount, zeroing any memory in the offset.
void ZeroByOffset(u64 size, u32 offset, GameMemory* game_memory) 
{
    u64 bytes = size * offset;
    u8* seek = (u8*)game_memory->next_available;
    if (bytes < 0) {
        for (i32 a = 0; a > bytes; a -= 1) {
            *seek = 0;
            seek -= 1;
        }
    } else {
        for (i32 a = 0; a < bytes; a += 1) {
            *seek = 0;
            seek += 1;
        }
    }

    game_memory->next_available += bytes;
    game_memory->permanent_storage_remaining -= bytes;
}

// perf: table of squares to cut out multiplications.
static v2f64 RenderSphericallySquared(i64 radius, i64 x, i64 y) 
{
    // note: rect to sphere conversion involves a square root:
    //       sphere_vector = unit_rect_vector * sphere_radius
    //       or without a square root:
    //       sphere_vector^2 = unit_rect_vector^2 * sphere_radius
    //       but since we are looking for sphere_vector, but not squared, we need to use the square root 
    //          for now...
    v3i64 rect_vector = {x, y, radius};
     
    i64 rect_x_sq = x * x;
    i64 rect_y_sq = y * y;
    i64 radius_squared = radius * radius;

    f64 distance_rect_squared = (f64)(rect_x_sq + rect_y_sq + radius_squared);

    v2f64 unit_rect_squared = {
        rect_x_sq / distance_rect_squared,
        rect_y_sq / distance_rect_squared,
    };

    v2f64 spherical_position_squared = {
        unit_rect_squared.x * radius_squared,
        unit_rect_squared.y * radius_squared,
    };

    return(spherical_position_squared);
}

// note: famous inverse square method used in Quake III.
// https://en.wikipedia.org/wiki/Fast_inverse_square_root#Overview_of_the_code
f32 Q_rsqrt(f32 number)
{
    i32 i;
	f32 x2, y;
	const f32 threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * (i32 * ) &y;                         // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck? 
	y  = * ( f32 * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

// bug: Q_rsqrt produces some slightly lacking precision; instead of normalizing (2^0.5, 0, 0) to (1, 0, 0), this 
//      method normalized instead to (.99999898672, 0, 0) approximately. 
static v3f64 NormalizeVector(v3f64 vector)
{
    f64 vector_x_sq = vector.x * vector.x;
    f64 vector_y_sq = vector.y * vector.y;
    f64 vector_z_sq = vector.z * vector.z;
    f32 distance_quotient = Q_rsqrt((f32)(vector_x_sq + vector_y_sq + vector_z_sq));

    return { vector.x * distance_quotient, vector.y * distance_quotient, vector.z * distance_quotient };
}

static SimpList<f64> sqrt_table = {};

// note: takes a maximum value for what 'x' and 'y' may be. Assumes a constant value for radius.
static void GenerateSqrtTable(u32 x, u32 y, u32 radius, GameMemory* game_memory) 
{
    u64 radius_squared = radius * radius;
    sqrt_table = NewList<f64>(x * y, game_memory);
    for (u64 counter = 0; counter < x; counter += 1) {
        for (u64 counter_2 = 0; counter_2 < y; counter_2 += 1) {
            u64 rect_x_sq = counter * counter;
            u64 rect_y_sq = counter_2 * counter_2;

            // Set<f64>(sqrt_table, counter + counter_2, Q_rsqrt((f32)(rect_x_sq + rect_y_sq + radius_squared)));
            Set<f64>(sqrt_table, counter + counter_2, pow((f64)(rect_x_sq + rect_y_sq + radius_squared), 0.5f));
        }
    }
}

static v2f64 RenderSpherically2D(i64 radius, i64 x, i64 y) 
{
    // note: rect to sphere conversion involves a square root:
    //       sphere_vector = unit_rect_vector * sphere_radius
    v3i64 rect_vector = {x, y, radius};
     
    i64 rect_x_sq = x * x;
    i64 rect_y_sq = y * y;
    i64 radius_squared = radius * radius;

    f64 distance_rect_squareroot = Q_rsqrt((f32)(rect_x_sq + rect_y_sq + radius_squared));
    // f64* distance_rect_squareroot = Get(sqrt_table, x + y);

    v2f64 unit_rect_squared = {
        x *distance_rect_squareroot,
        y *distance_rect_squareroot,
    };

    v2f64 spherical_position_squared = {
        unit_rect_squared.x * radius,
        unit_rect_squared.y * radius,
    };

    return(spherical_position_squared);
}

// note: given some point, will recalculate its position as though it were projected on a sphere whose center
//       is (z_offset - sphere_radius) away from the point.
static v3f64 RenderSpherically(i64 sphere_radius, v3f64 point, i64 z_offset) 
{
    // note: rect to sphere conversion involves a square root:
    //       sphere_vector = unit_rect_vector * sphere_radius
    f64 rect_x_sq = point.x * point.x;
    f64 rect_y_sq = point.y * point.y;
    f64 rect_z_sq = point.z * point.z;
    f64 radius_squared = point.z * point.z;

    f64 distance_rect_squareroot = Q_rsqrt((f32)(rect_x_sq + rect_y_sq + rect_z_sq));

    // study: this should be FASTER than above, but produces a sort of visual seam.
    // f64* distance_rect_squareroot = Get(sqrt_table, x + y);

    v3f64 unit_rect_squared = {
        point.x * distance_rect_squareroot,
        point.y * distance_rect_squareroot,
        point.z * distance_rect_squareroot,
    };
 
    v3f64 spherical_position = {
        unit_rect_squared.x * sphere_radius,
        unit_rect_squared.y * sphere_radius,
        unit_rect_squared.z * sphere_radius,
    };

    return(spherical_position);
}

// Allocate space for some size * number bytes and return the next available space in memory.
static void AdjustMemory(u64 size, u64 number, GameMemory* game_memory, void** to_allocate)
{
    // to_allocate is now referencing space at 
    *to_allocate = game_memory->next_available;

    ZeroByOffset(size, (u32)number, game_memory);
}

// assume that these vertices are going to get here somehow in a way that includes being distorted.
// note:   we have effectively 2 million operations in turns of the rendering per pixel.
//         duplicating this workflow in intensity would require 2 million triangles.... that is kinda crazy.
//         even just having 20 thousand triangles seems like a lot.  
//         what we are actually doing on a per-pixel basis with some rectangle is actually A LOT compared to 
//         potentially the rest of the game.
static SimpList<Polyhedron> MorphPolyhedra(SimpList<Polyhedron> polyhedra, GameMemory* game_memory) 
{
    SimpList<Polyhedron> result_polys = {};
    AdjustMemory(sizeof(Polyhedron), polyhedra.length, game_memory, (void**)&(result_polys.array));
    result_polys.length += polyhedra.length;
    for (i32 a = 0; a < polyhedra.length; a += 1) {
        result_polys.array[a].color = polyhedra.array[a].color;

        AdjustMemory(sizeof(v3f64), polyhedra.array[a].vertices.length,
                game_memory, (void**)&(result_polys.array[a].vertices.array));
        result_polys.array[a].vertices.length += polyhedra.array[a].vertices.length;
        for (i32 b = 0; b < polyhedra.array[a].vertices.length; b += 1) { 
            if (polyhedra.array[a].morph_status == SPHERE) {
                result_polys.array[a].vertices.array[b] = RenderSpherically(1000, 
                        polyhedra.array[a].vertices.array[b], 0);
            } else {
                result_polys.array[a].vertices.array[b] = polyhedra.array[a].vertices.array[b];
            }
        }

        AdjustMemory(sizeof(TriangleRefVertices), polyhedra.array[a].triangles.length,
                game_memory, (void**)&(result_polys.array[a].triangles.array));
        result_polys.array[a].triangles.length += polyhedra.array[a].triangles.length;
        for (i32 b = 0; b < polyhedra.array[a].triangles.length; b += 1) { 
            // Use the offsets calculated from the fact that all vertices are just being stored some offset from their
            // first address in memory.
            u64 ptr_offset_1 = polyhedra.array[a].triangles.array[b].verts[0] - polyhedra.array[a].vertices.array;
            u64 ptr_offset_2 = polyhedra.array[a].triangles.array[b].verts[1] - polyhedra.array[a].vertices.array;
            u64 ptr_offset_3 = polyhedra.array[a].triangles.array[b].verts[2] - polyhedra.array[a].vertices.array;

            result_polys.array[a].triangles.array[b].verts[0] = result_polys.array[a].vertices.array + ptr_offset_1;
            result_polys.array[a].triangles.array[b].verts[1] = result_polys.array[a].vertices.array + ptr_offset_2;
            result_polys.array[a].triangles.array[b].verts[2] = result_polys.array[a].vertices.array + ptr_offset_3;
        }
    }

    return(result_polys);
}

static Line FindLineOfIntersection(Tri tri, Tri to_plane)
{
    Line result = {};

    return(result);
}

//todo: incomplete.
static bool IsColinear(v3f64 one, v3f64 two, v3f64 three) 
{
    return(0);
}

/// Split to_split into multiple triangles if test overlaps one of its lines. Splitting is along the line of 
/// intersection.
static SimpList<Tri> SplitTrianglesIfOverlap(Tri to_split, Tri test) 
{
    SimpList<Tri> new_triangles = {};

    // create infinite plane from 'test'
    // if there is an intersection line with that plane, than the triangles are overlapping.
    // if the triangles ARE overlapping, then we will return the new three triangles that 'to_split' will be split
    //       into. This polygon will be the two points calculated, and as well the OTHER two points of the 
    //       original triangle.
    //       From here we just pick the two polygon points to connect with one of the OTHER, and the second
    //       polygon point to connect with the OTHER two.
    //       Then, we will have our bisected triangle divided into 3 triangles. rendering these triangles
    //       along with the other triangle that will NOT need to be bisected now that these have been.

    Line line = FindLineOfIntersection(to_split, test);
    if (line.x.x == line.y.x && line.x.y == line.y.y) {
        // If line's points are equal, Triangles do not intersect.
        return(new_triangles); 
    }
    
    // Intersection line will be colinear with two of the three sides of the triangle.
    // the vertex that is in both lines will be the ONE that will be the first triangle.
    // the OTHER TWO vertices will be used with the intersection line as a 4-gon with which
    // to split correctly into two triangles sharing an edge.
    
    Tri new_tri = {};
    Tri second_tri = {};
    Tri third_tri = {};
    if (IsColinear(line.x, to_split.verts[0], to_split.verts[1]) &&
        IsColinear(line.y, to_split.verts[0], to_split.verts[1]))
    {
        new_tri.verts[0] = line.x;
        new_tri.verts[1] = line.y;
        new_tri.verts[2] = to_split.verts[2]; 
        second_tri.verts[0] = line.x;
        second_tri.verts[1] = line.y;
        second_tri.verts[2] = to_split.verts[0];
        third_tri.verts[0] = line.y;
        third_tri.verts[1] = to_split.verts[0];
        third_tri.verts[2] = to_split.verts[1];
    }
    else if(IsColinear(line.x, to_split.verts[0], to_split.verts[2]) &&
            IsColinear(line.y, to_split.verts[0], to_split.verts[2])) 
    {
        new_tri.verts[0] = line.x; 
        new_tri.verts[1] = line.y;
        new_tri.verts[2] = to_split.verts[1];
        second_tri.verts[0] = line.x;
        second_tri.verts[1] = line.y;
        second_tri.verts[2] = to_split.verts[0];
        third_tri.verts[0] = line.y;
        third_tri.verts[1] = to_split.verts[0];
        third_tri.verts[2] = to_split.verts[2];
    }
    else if(IsColinear(line.x, to_split.verts[1], to_split.verts[2]) &&
            IsColinear(line.y, to_split.verts[1], to_split.verts[2]))
    {
        new_tri.verts[0] = line.x; 
        new_tri.verts[1] = line.y;
        new_tri.verts[2] = to_split.verts[0];
        second_tri.verts[0] = line.x;
        second_tri.verts[1] = line.y;
        second_tri.verts[2] = to_split.verts[1];
        third_tri.verts[0] = line.y;
        third_tri.verts[1] = to_split.verts[1];
        third_tri.verts[2] = to_split.verts[2];
    }

    new_triangles.array[0] = new_tri;
    new_triangles.array[1] = second_tri;
    new_triangles.array[2] = third_tri;

    return(new_triangles);
}

struct Tri_Color
{
    TriangleRefVertices triangle;
    Color color;
};

static f64 Square(f64 num) 
{
    return(num * num);
}

static f64 DistanceSquared(v3f64 one, v3f64 two)
{
    // note: overflow likely
    f64 one_two_x = two.x - one.x;
    f64 one_two_y = two.y - one.y;
    f64 one_two_z = two.z - one.z;

    f64 x_squared = one_two_x * one_two_x;
    f64 y_squared = one_two_y * one_two_y;
    f64 z_squared = one_two_z * one_two_z;

    return(x_squared + y_squared + z_squared);
}

static v3f64 FindClosestTriVert(TriangleRefVertices triangle, v3f64 test)
{
    f64 one_dist = DistanceSquared(test, *(triangle.verts[0]));
    f64 two_dist = DistanceSquared(test, *triangle.verts[1]);
    f64 three_dist = DistanceSquared(test, *triangle.verts[2]);

    if (one_dist < two_dist && one_dist < three_dist) 
    {
        return(*triangle.verts[0]);
    } 

    if (two_dist < one_dist && two_dist < three_dist) 
    {
        return(*triangle.verts[1]);
    }

    // perf: do not need this last if, as either third vertex is shortest, or they are all the same distance.
    //       either way, we return the last vertex.
    if (three_dist < one_dist && three_dist < two_dist) 
    {
        return(*triangle.verts[2]);
    }

    // All vertices are equal.
    return(*triangle.verts[2]);
}

// note: does not deep copy. 
template <typename T>
static void SwapValuesAtAddresses(SimpList<T> list, u64 one, u64 other)
{
    T swap = list.array[one];
    list.array[one] = list.array[other];
    list.array[other] = swap;
}

// note: orders vertices according to observer normal. the result is a list which one can draw from
static SimpList<Tri_Color> OrderVerts(SimpList<Polyhedron> polys, v3f64 observer_normal, GameMemory* game_memory)
{
    SimpList<Tri_Color> ordered_triangles = {};

    u64 total_tri_count = 0;
    for (i32 a = 0; a < polys.length; a += 1) {
        total_tri_count += polys.array[a].triangles.length;
    }

    // note: we will have to walk this back below since we are going to be removing bisected triangles.
    AdjustMemory(sizeof(Tri_Color), total_tri_count, game_memory, (void**)&(ordered_triangles.array));

    // Find all bisected triangles. For each bisected triangle, split the triangles, and add the new triangles to 
    // result. For each non-bisected, just add it.

    // for polyhedra
    // for triangle in polyhedra
    // for polyhedra
    // for triangle in polyhedra
    SimpList<TriColorNoRef> additional_triangles = {};
    i32 memory_offset_triangle_removal = 0;
    for (i32 a = 0; a < polys.length; a += 1) {
        for (i32 b = 0; b < polys.array[a].triangles.length; b += 1) {
            bool triangle_bisected = false;
            for (i32 c = a; c < polys.length; c += 1) {
                for (i32 d = b; d < polys.array[c].triangles.length; d += 1) {
                    Tri one = {};
                    one.verts[0] = *(polys.array[a].triangles.array[b].verts[0]);
                    one.verts[1] = *(polys.array[a].triangles.array[b].verts[1]);
                    one.verts[2] = *(polys.array[a].triangles.array[b].verts[2]);

                    Tri two = {};
                    two.verts[0] = *(polys.array[c].triangles.array[d].verts[0]);
                    two.verts[1] = *(polys.array[c].triangles.array[d].verts[1]);
                    two.verts[2] = *(polys.array[c].triangles.array[d].verts[2]);

                    SimpList<Tri> triangles = SplitTrianglesIfOverlap(one, two);

                    // note: this may necessitate some hairy situation, putting all these new triangles bisected back
                    //       into the triangles list because they may themselves be bisected by something else, and
                    //       starting this entire quad-loop over from start .
                    if (triangles.length != 0) {
                        // if some plane has ANY of its lines intersecting, we will run the same 
                        // IntersectionOfTwoPlanes 
                        //       but in which the other plane is now of infinite size. This will get the other point, 
                        //       and break the bisected triangle into 1 triangle and 1 polygon(4).

                        // add new triangles to ordered_triangles
                        for (i32 z = 0; z < triangles.length; z += 1) {
                            TriColorNoRef convert = {};
                           
                            convert.triangle.verts[0] = triangles.array[z].verts[0];
                            convert.triangle.verts[1] = triangles.array[z].verts[1];
                            convert.triangle.verts[2] = triangles.array[z].verts[2];
                            convert.color = polys.array[a].color;

                            additional_triangles.array[additional_triangles.length + z] = convert;
                        }

                        ordered_triangles.length += triangles.length;

                        // increment the amount to move memory back later.
                        memory_offset_triangle_removal += 1;
                        triangle_bisected = true;
                        break;
                    }
                }

                if (triangle_bisected == true) {
                    break;
                }
            }

            if (triangle_bisected == false) {
                ordered_triangles.array[b].triangle = polys.array[a].triangles.array[b];
                ordered_triangles.array[b].color = polys.array[a].color;
                ordered_triangles.length += 1;
            }
        }
    }

    // walk back next available by freeing the final memory_offset_triangle_removal amount of Tri_color
    ZeroByOffset(sizeof(Tri_Color), memory_offset_triangle_removal * -1, game_memory);

    // Append additional_triangles to end of ordered_triangles.
    memcpy(game_memory->next_available, additional_triangles.array, 
            sizeof(TriColorNoRef) * additional_triangles.length);
    game_memory->next_available += sizeof(TriColorNoRef) * additional_triangles.length;
    game_memory->permanent_storage_remaining -= sizeof(Tri_Color) * additional_triangles.length;

    // Sort triangles by nearest vert to observer_normal.
    for (i32 a = 0; a < ordered_triangles.length; a += 1) {
        v3f64 outer = FindClosestTriVert(ordered_triangles.array[a].triangle, observer_normal); 
        
        f64 minimal_outer = DistanceSquared(outer, observer_normal);
        i32 index_to_swap = a;
        for (i32 b = a + 1; b < ordered_triangles.length; b += 1) {
            // find the minimal.
            v3f64 inner = FindClosestTriVert(ordered_triangles.array[b].triangle, observer_normal);
            f64 minimal_inner = DistanceSquared(inner, observer_normal);
            if (minimal_inner < minimal_outer) {
                minimal_outer = minimal_inner;
                index_to_swap = b;
            }
        }

        SwapValuesAtAddresses(ordered_triangles, a, index_to_swap);
    }

    return(ordered_triangles);
}

static inline v3f64 MultiplyVector(f64 scalar, v3f64 vector)
{
    return { vector.x * scalar, vector.y * scalar, vector.z * scalar };
}

// Does no normalization, does not return a unit vector.
// study: separate geometry methods into a separate file? 
static v3f64 SurfaceNormal(v3f64 one, v3f64 two)
{
    return { one.y * two.z - one.z * two.y, one.z * two.x - one.x * two.z, one.x * two.y - one.y * two.x };
}

static f64 DotProduct(v3f64 one, v3f64 two)
{
    return (one.x * two.x + one.y * two.y + one.z * two.z);
}

// Using Cramer's Formula, integrating this answer: https://stackoverflow.com/a/42752998
// Triangle verts are multiplied by some scale value, set to 1 for no scaling.
bool intersect_triangle(Line line, TriangleRefVertices triangle, f64 scale) 
{ 
    // study: consider overloading '*' operator for scalars and vectors.
    Tri tri = { 
        MultiplyVector(scale, *triangle.verts[0]),
        MultiplyVector(scale, *triangle.verts[1]),
        MultiplyVector(scale, *triangle.verts[2]),
    };

    v3f64 E1 = { 
        tri.verts[1].x - tri.verts[0].x,
        tri.verts[1].y - tri.verts[0].y,
        tri.verts[1].z - tri.verts[0].z,
    };

    v3f64 E2 = { 
        tri.verts[2].x - tri.verts[0].x,
        tri.verts[2].y - tri.verts[0].y,
        tri.verts[2].z - tri.verts[0].z,
    };

    v3f64 N = SurfaceNormal(E1, E2);

    v3f64 line_to_zero = {
        line.y.x - line.x.x,
        line.y.y - line.x.y,
        line.y.z - line.x.z
    };

    f64 line_x_squared = line_to_zero.x * line_to_zero.x;
    f64 line_y_squared = line_to_zero.y * line_to_zero.y;
    f64 line_z_squared = line_to_zero.z * line_to_zero.z;

    f32 inverted_line_distance = Q_rsqrt((f32)(line_x_squared + line_y_squared + line_z_squared));

    // perf: can precompute this assuming the per chosen clip_range (only once if the clip_range does not change)
    v3f64 line_direction = {
        line_to_zero.x * inverted_line_distance,
        line_to_zero.y * inverted_line_distance,
        line_to_zero.z * inverted_line_distance,
    };

    f64 det = DotProduct(line_direction, N) * -1;

    f64 invdet = 1.0 / det;
    v3f64 AO = {
        line.x.x - tri.verts[0].x, 
        line.x.y - tri.verts[0].y, 
        line.x.z - tri.verts[0].z, 
    };

    v3f64 DAO = SurfaceNormal(AO, line_direction);

    f64 u =  DotProduct(E2,DAO) * invdet;
    f64 v =  DotProduct(E1,DAO) * invdet * -1;
    f64 t =  DotProduct(AO,N) * invdet; 

    return ((det >= (1 * pow(10, -6)) || det <= (-1 * pow(10, -6)))
            && t >= 0.0 && u >= 0.0 && v >= 0.0 && (u+v) <= 1.0);
}

// Will return either the Color of the first Triangle containing this pixel, or empty_space.
static Color TriangleContainingPixel(SimpList<Tri_Color> tris, i32 pixel_x, i32 pixel_y, Color empty_space, f64 scale)
{
    u32 clip_range = 20;
    
    Line line = {
        { (f64)pixel_x, (f64)pixel_y, 0 },
        { (f64)pixel_x, (f64)pixel_y, (f64)clip_range },
    };

    for (i32 a = 0; a < tris.length; a += 1) {
        // does the line intersect with the triangle
        if (intersect_triangle(line, tris.array[a].triangle, scale)) {
             return(tris.array[a].color);
        }
    }

    return(empty_space);
}

// perf: frame drops go away when compiler option -Og (or its super-set -O2) are declared.
//       this is doing loop optimization, and perhaps some more things. 
// perf: single threaded. can distribute the load.
// study: how to "look" at something.
static void Render(GameOffscreenBuffer *buffer, i64 x_offset, i64 y_offset, i64 z_offset, SimpList<Tri_Color> tris, 
        v3f64 observer_normal) 
{
    i32 zoom = (i32)(z_offset / GRID_SIZE + 256);
    f32 zoom_apply = 256.0f / zoom;

    // zoom_apply = 1;

    f64 scale = 256;

    // for each pixel ask, does any triangle contain this pixel? start from the front of the list of tris which
    //      is ordered. if the answer is 'yes', color it with that triangles color, and continue to next pixel.

    int pitch = buffer->width * buffer->bytes_per_pixel;
    u8* row = (u8*)buffer->memory;
    for (i32 y = 0; y < buffer->height; y += 1) {
        u32* pixel = (u32*)row;
        for (i32 x = 0; x < buffer->width; x += 1) {
            i32 color = 0;
            Color pixel_result = TriangleContainingPixel(tris, x, y, { 255, 0, 255, 255 }, scale);

            color = (u8)((pixel_result.a + (x_offset / GRID_SIZE)) * zoom_apply);
            color = color << 8;
            color += (u8)((pixel_result.red - (y_offset / GRID_SIZE)) * zoom_apply);
            color = color << 8;
            color += (u8)((pixel_result.green - (y_offset / GRID_SIZE)) * zoom_apply);
            color = color << 8;
            color += (u8)((pixel_result.blue - (y_offset / GRID_SIZE)) * zoom_apply);

            *pixel = color;

            pixel += 1;
        }

        row += pitch;
    }
}

// note: rotations are in a sphere-like fashion and preserve distances between vertices. In effect, a cube with
//       a rotation applied will still look exactly like the same cube you passed this method. 
//       There are other options one could do (rotation as though along the surface of a cube) which would produce
//       altered results.
// study: quaternions!!!
static void RotatePolyhedra(SimpList<Polyhedron> polys, v3f64 observer_now, v3f64 rotation_default)
{
    // No rotation has occurred.
    if     (rotation_default.x == observer_now.x && 
            rotation_default.y == observer_now.y && 
            rotation_default.z == observer_now.z
    ) {
       return; 
    } 

    // using Rodrigues' rotation formula following the notion of defining rotation as an axis-angle representation.
    // https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
    v3f64 planar_normal = SurfaceNormal(rotation_default, observer_now);

    // value must be normalized. 
    planar_normal = NormalizeVector(planar_normal);

    f64 angle_of_rotation = 0;
    if (planar_normal.x == 0 && planar_normal.y == 0 && planar_normal.z == 0) {
        // degree of rotation is 180, as lines are parallel. 
        angle_of_rotation = PI;
    } else {
        // Doing a bunch of trig will reduce finding the angle between the two isosceles sides to be the following.
        // arccos [(b² + c² - a²)/(2bc)], where 'a' is the non-isosceles side.
        // note: the isosceles sides are both '1' length.
        f64 isosceles_one = 1;
        f64 isosceles_two = 1;
        f64 non_isosceles_side = Square(observer_now.x - rotation_default.x) + 
            Square(observer_now.y - rotation_default.y) + 
            Square(observer_now.z - rotation_default.z);

        angle_of_rotation = acos((2 - non_isosceles_side) / 2);
    }

    f64 angle_sin = sin(angle_of_rotation);
    f64 angle_cos = cos(angle_of_rotation);

    // change value of polyhedron vertex.
    // learn the derivation here: https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula#Statement
    for (i32 a = 0; a < polys.length; a += 1) {
        for (i32 b = 0; b < polys.array[a].vertices.length; b += 1) {
            f64 first_term_x = polys.array[a].vertices.array[b].x * angle_cos;
            f64 first_term_y = polys.array[a].vertices.array[b].y * angle_cos;
            f64 first_term_z = polys.array[a].vertices.array[b].z * angle_cos;

            v3f64 second_term = SurfaceNormal(planar_normal, polys.array[a].vertices.array[b]);
            second_term.x = second_term.x * angle_sin;
            second_term.y = second_term.y * angle_sin;
            second_term.z = second_term.z * angle_sin;

            v3f64 third_term = planar_normal;
            f64 dot_product = DotProduct(planar_normal, polys.array[a].vertices.array[b]);
            third_term.x = third_term.x * (1 - angle_cos) * dot_product;
            third_term.y = third_term.y * (1 - angle_cos) * dot_product;
            third_term.z = third_term.z * (1 - angle_cos) * dot_product; 
            
            polys.array[a].vertices.array[b] = {
                first_term_x + second_term.x + third_term.x, 
                first_term_y + second_term.y + third_term.y, 
                first_term_z + second_term.z + third_term.z };
        }
    }
}

// perf: sqrt floats.
static v3f64 NormalizedVector(v3f64 vector) 
{
    f64 inverse_distance = Q_rsqrt((f32)(Square(vector.x) + Square(vector.y) + Square(vector.z)));

    return { vector.x * inverse_distance, vector.y * inverse_distance, vector.z * inverse_distance };
}

static void AssembleDrawableRect(GameOffscreenBuffer *buffer, i64 x_offset, i64 y_offset, i64 z_offset, 
       SimpList<Polyhedron> polyhedra, GameMemory* game_memory)
{
    v3f64 rotation_default = { 0, 1, 0 };

    // note: normal cube should now have one of its edges facing us.
    v3f64 world_rotation_now = NormalizedVector({ 1, 0, 0 }); 

    // Distort all polyhedra marked for distortion.
    SimpList<Polyhedron> morphed_polys = MorphPolyhedra(polyhedra, game_memory);

    // "Rotating" morphed_polys relative to user_position.
    RotatePolyhedra(morphed_polys, world_rotation_now, rotation_default);

    // perf: if a triangle is in front of me, and within a certain distance, I will draw it!
    //       else, i will remove it from morphed_polys....

    // research: needs more! Assemble into some draw order.
    //           problems to solve: overlapping triangles. we need to be able to break down some triangle in 
    //           such a way that given some oberver_normal, we can use the line which overlaps some triangle
    //           to break that triangle down into constituent pieces. 
    // break into a list of Triangles which can be drawn in order.
    v3f64 observer_normal = { 0, 0, -4 };
    SimpList<Tri_Color> tris = OrderVerts(morphed_polys, observer_normal, game_memory);

    // Draw.
    Render(buffer, view_move_x, view_move_y, view_move_z, tris, observer_normal);

    // Free all the memory we used up.
    // ZeroMemory(tris);
    ZeroMemory(morphed_polys.array, morphed_polys.length, sizeof(Polyhedron));
}

// note: angel_inputs and final_inputs are global.
static void SetDefaultInputs(std::vector<AngelInput> inputs)
{
    for (int i = 0; i < inputs.size(); i += 1) {
        AngelInput new_angel = inputs.at(i);
        angel_inputs.inputs[i] = new_angel; // Copy.

        // Default values for final_input.
        Input new_in;
        new_in.lr = &angel_inputs.inputs[i].stick_1;
        new_in.z = &angel_inputs.inputs[i].stick_2;
        new_in.interact_mask = 8;
        final_inputs[i] = new_in;
    }

    angel_inputs.next_available_index = (u32)inputs.size();
}

static int NumberKeyFirstDown(int bitflag) 
{
    int result = -1;

    if (bitflag & KEY_0) {
        result = 0;
    } else if (bitflag & KEY_1) {
        result = 1;
    } else if (bitflag & KEY_2) {
        result = 2;
    } else if (bitflag & KEY_3) {
        result = 3;
    } else if (bitflag & KEY_4) {
        result = 4;
    } else if (bitflag & KEY_5) {
        result = 5;
    } else if (bitflag & KEY_6) {
        result = 6;
    } else if (bitflag & KEY_7) {
        result = 7;
    } else if (bitflag & KEY_8) {
        result = 8;
    } else if (bitflag & KEY_9) {
        result = 9;
    }

    return(result);
}

inline int Calculate10DigitValue(int base, int digit_number)
{
    return(int)(base * pow(10, digit_number));
}

inline bool IsKeyDown(u32 key, u32 device_id) 
{
    return((angel_inputs.inputs[device_id].keys & key) == key);
}

inline bool IsKeyFirstDown(u32 key, u32 device_id)
{
    return((angel_inputs.inputs[device_id].keys_first_down & key) == key);
}

inline bool IsKeyFirstUp(u32 key, u32 device_id)
{
    return((angel_inputs.inputs[device_id].keys_first_up & key) == key);
}

// study: group as something to reference in developer code.
static int dev_stage_id;
static int digit_number = 0;
static bool paused;

internal void ProcessInputs(GameOffscreenBuffer *buffer, std::vector<AngelInput> inputs, GameState* game_state, 
        GameMemory* game_memory, PlatformServices services) 
{
    for (int i = 0; i < angel_inputs.next_available_index; i += 1) {
        angel_inputs.inputs[i].stick_1 = inputs[i].stick_1;
        angel_inputs.inputs[i].stick_2 = inputs[i].stick_2;
        angel_inputs.inputs[i].keys = inputs[i].keys;
        angel_inputs.inputs[i].keys_first_down = inputs[i].keys_first_down;
        angel_inputs.inputs[i].keys_first_up = inputs[i].keys_first_up;

        // This is how we check if some game defined behavior is occcuring.
        u16 interact_down = angel_inputs.inputs[i].keys & final_inputs[i].interact_mask;
        if (interact_down == final_inputs[i].interact_mask) {
            // Switch left and right control stick. 
            final_inputs[i].lr = &angel_inputs.inputs[i].stick_2;
            final_inputs[i].z = &angel_inputs.inputs[i].stick_1;
        }

        // note: keys are little endian! this means 'L12' means Load 21.
#if NEWGAME_INTERNAL
        if (i == 0) {
            bool result = IsKeyFirstUp(KEY_P, i);
            if (result) {
                // Pause the game. 
                paused = !paused;
            }

            if (IsKeyDown(KEY_L, i)) {
                // If numeric key down, continue adding up the following numbers.
                // note: we just ignore if you hit non-numeric keys while L is held down.
                int digit_held = NumberKeyFirstDown(angel_inputs.inputs[i].keys_first_down);
                if (digit_held != -1) {
                    dev_stage_id += Calculate10DigitValue(digit_held, digit_number);
                    digit_number += 1;
                }
            } else if (IsKeyFirstUp(KEY_L, i)) {
                if (paused == false) {
                    LoadArea(dev_stage_id, game_state, game_memory, services, &rumble_patterns[0]);
                }

                dev_stage_id = 0;
                digit_number = 0;
            }
        }
#endif
    }
}

// note: To play a song. Loads the song if it is not yet loaded. 
void PlayASong(char* name, GameMemory* game_memory, PlatformServices services) 
{
    SoundAssetTable* sound_ptr = &sound_assets;
    services.load_sound("Eerie_Town.wav", &sound_ptr, game_memory); // Points our SoundAssetTable to the new head.
    
    services.start_playing("Eerie_Town.wav", &sound_assets, game_memory);
}

// note: to rumble controllers, pass a pointer to the pattern. Pattern will be mutated platform side, so make sure 
//       to send the same pattern data whenever you need to continue rumbling. 
void RumbleController(u32 device_index, DoubleInterpolatedPattern* pattern, PlatformServices services)
{
    services.rumble_controller(device_index, pattern);
}
 
static void CalculateNextFrame(GameState* game_state) 
{
    if (game_state->current_path.current_time < game_state->current_path.length) {
        view_move_x = Get(game_state->current_path.values, game_state->current_path.current_time)->x;
        view_move_y = Get(game_state->current_path.values, game_state->current_path.current_time)->y;
        view_move_z = Get(game_state->current_path.values, game_state->current_path.current_time)->z;

        // note: unit of time is frames, not milliseconds.
        game_state->current_path.current_time += 1;
    }
}

static void InitGame(GameState* game_state, GameMemory* game_memory, PlatformServices services,  
        std::vector<AngelInput> inputs, GameOffscreenBuffer* buffer) 
{
    // Generate some constant value lookup tables.
    GenerateSqrtTable(buffer->width, buffer->height, buffer->width, game_memory); 

    // Load from initial area. 
    current_area = LoadArea(0, game_state, game_memory, services, &rumble_patterns[0]);

    auto_save_timer = 0;

    SetDefaultInputs(inputs);
    game_memory->initialized = true;
}

extern "C" void GameUpdateAndRender(GameOffscreenBuffer* buffer, std::vector<AngelInput> inputs, GameState* game_state,
        GameMemory* platform_memory, PlatformServices services, f32 seconds_per_frame)
{
    GameMemory* game_memory = platform_memory;
    if (game_memory->initialized == false) {
        InitGame(game_state, game_memory, services, inputs, buffer);
    }

    ProcessInputs(buffer, inputs, game_state, game_memory, services);
    
    if (paused == false) {
        // note: everything in here is what we want to be pausable.

        // todo: There exist a list of patterns occurring at any one time. 
        //       One example is game_state->current_path. Another will be every object undergoing some change over 
        //       time.
        //       We should group all of these changes over time so that we can apply some meaningful translations, etc,
        //       before rendering.
    }

    CalculateNextFrame(game_state);

    // study: casey remarks. how to make renderer faster!
    // study: Change render weird gradient, white/yellow patterns.
    AssembleDrawableRect(buffer, view_move_x, view_move_y, view_move_z, current_area.friendlies, game_memory);
}

extern "C" void PauseAudio(char* name, PlatformServices services) 
{
    services.pause_audio(name, &sound_assets);
}

extern "C" void ResumeAudio(char* name, PlatformServices services) 
{
    services.resume_audio(name, &sound_assets);
}

enum AudioCode 
{
    PLAY,
    PAUSE,
    STOP,
    STOP_ALL
};

struct AudioMessage 
{
    AudioCode code;
    AudioAsset* data;
};

struct AudioMessageQueue 
{
    AudioMessage message[100];
};

static AudioMessageQueue audio_queue = {};
