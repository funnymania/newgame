#include "newgame.h"

// note: assuming camera is at origin, with a direction vector positive in z, and wideness of 0.5.
bool IsTriangleInCamera(Tri* triangle, Camera camera, Transform tra)
{
    for (int counter = 0; counter < 3; counter += 1) {
        float distance = triangle->verts[counter].z + tra.pos.z - camera.pos.z;
        float frustrum_extent = distance * camera.wideness; 
        
        // Vertex is behind camera.
        if (distance < 0) {
            continue;
        }

        float cam_range_x = 1.78 / 2;
        float cam_range_y = 1 / 2;
        if (triangle->verts[counter].x + tra.pos.x < frustrum_extent * (camera.pos.x + cam_range_x)
            && triangle->verts[counter].x + tra.pos.x > frustrum_extent * (camera.pos.x - cam_range_x)
            && triangle->verts[counter].y + tra.pos.y < frustrum_extent * (camera.pos.y + cam_range_y)
            && triangle->verts[counter].y + tra.pos.y > frustrum_extent * (camera.pos.y - cam_range_y)
        ) {
           return(true); 
        }
    }

    return(false);
}

void RenderModels(RunningModels* models, Camera camera) 
{
    // Downward, angled towards viewer to their left.
    v3_float light = {};
    light.x = -1;
    light.y = -1;
    light.z = 1;

    // for each model.....
    // for each triangle.....
    // 1. if tri is in camera view, we need to render it. 
    // 2. how far is it (x, y, and z) from camera? (this determines where on the screen it is, and how much space it 
    //       takes up.
    // 3. cross multiply tri with camera to determine the camera_view_shape of tri.
    // 4. "shrink" size relative to z distance from camera.
    Obj* model_ptr = models->models;
    int counter = 0;
    while (counter < models->models_len) {
        Tri* triangle_ptr = model_ptr->triangles;
        int tri_counter = 0;
        while (tri_counter < model_ptr->triangles_len) {
            // From camera "center", take a vertex and test if, give it's Z-distance from camera, it is within
            // the rectangle that is the camera's "view" at that point!
            // there will be a formula such that at every 1 unit distance from camera, frustrum expands x and y
            // relative to aspect ratio. since our aspect ratio is 1280 by 720, this would mean... 
            // for every Z unit, frustrum increases 1.78y and 1x.
            if (IsTriangleInCamera(triangle_ptr, camera, model_ptr->tra)) {
                // given distance from camera, compute "size" and placement on the screen of tri
                // morph shape of tri given angle of camera and rotation of model containing tri. 
            }

            tri_counter += 1;
            triangle_ptr += 1;
        }

        counter += 1;
        model_ptr += 1;
    } 
    
    // calculate lighting to determine what each triangle should be colored as.
    
    // paint that imagery.
    // optionally send to GPU.
}
