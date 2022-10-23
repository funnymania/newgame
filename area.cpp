#include "platform_services.h"
#include "polyhedron.h"
#include <string>               // this is for isspace.

#define GRID_SIZE 32768

#define POINT4_i64(x, y, z, t) \
    { (x) * (GRID_SIZE), (y) * (GRID_SIZE), (z) * (GRID_SIZE), t }

// Contains Assets which are loaded, stored in Area files.
struct Area 
{
    SimpList<Polyhedron> friendlies; 
};

static void SwitchArea(char* name, GameMemory* game_memory)
{
    // Check to see if this level is already in memory.
    //       if so switch..
    //       else LoadArea() and then switch
}

void GameMemoryFree(u8* obj_ptr, u32 obj_count, u32 obj_size) 
{
    u8* tmp = (u8*)obj_ptr;
    for (int counter = 0; counter < obj_size * obj_count; counter += 1) 
    {
        *tmp = 0;
        tmp += 1;
    }
}

// terminology: 'level' is a developer term, and just refers to an assembling of data/assets that we can load up
//                  quickly and examine/change.
//              'the game' is that which represents the total assemblage of everything as it would be at any point
//                  in time. 'the game' is not broken down into 'levels', 'level's are just convenient structures
//                  for loading up and editing specific parts of 'the game'.

// note: sets up the level for playing
//       this shouldn't be determining what is actually on screen. this should be determining what to LOAD
//       for a developer shortcut.
//       THEREFORE a LEVEL is entirely a dev term, and so we should treat it as such.
//       In reality, the game_state is not affected by these levels and any associated level files.
//       So, the 'path' as it were here being somewhat relevant should be tied in with a game_state that is
//       meaningfully passed around.
//       THAT GAMESTATE is what is setup, not this LevelResults thing.
//
//
//       pre-files: staging occurs, levels are saved so that they can be accessed between sessions.
//       level files: saved to files which are overwritten on some command.
//           
//       stage things. can persist aspects of gamestate to some file for loading from while developing
//                   can also save things to file such that they are persisted in the game.
//
//       you don't want to decouple your game into separate files that do not live together. 
//       
//       "the game" itself will auto-save by keeping track of everything placed everywhere. 
//
//       keyboard commands: 'sg' is save game. 'sb' is save level. 'sb' will only save the current level.
//               to make changes to another level you would need to 'l#' the level. 
void SaveLevel() 
{

}

// note: Gamestate consists in staged things.
void Stage(GameState* game_state, GameMemory* game_memory) 
{
    // path.
    SimpList<v4i64> path = {};
    AddToList<v4i64>(&path, POINT4_i64(0, 0, 0, 0), game_memory);
    AddToList<v4i64>(&path, POINT4_i64(0, 0, 75, 75), game_memory);
    AddToList<v4i64>(&path, POINT4_i64(0, 0, 75, 110), game_memory);
    AddToList<v4i64>(&path, POINT4_i64(0, 0, 115, 150), game_memory);
    AddToList<v4i64>(&path, POINT4_i64(0, 0, 265, 300), game_memory);
    AddToList<v4i64>(&path, POINT4_i64(0, 0, 415, 450), game_memory);

    game_state->current_path = InterpolatedPattern::Create(path, game_memory);

    GameMemoryFree((u8*)path.array, 6, sizeof(v4i64)); 
}

struct char_size 
{
    char* ptr;
    u64 size;
};

// Returns the first string of alpha-numeric or 0 if starts with non-alpha-numeric.
char_size ReadAlphaNumericUntilWhiteSpace(char* start, char* eof) 
{
    char_size res = {};
    u64 size = 0;

    // pass to get the size of the number.
    char* tmp_start = start;
    while (std::isspace(*tmp_start) == 0 && tmp_start != eof) { // perf: requires dependencies.
        size += 1;
        tmp_start += 1;
    }

    // Just return 0 if white space is leading character.
    if (size == 0) {
       return(res); 
    }

    // Copy the strings.
    char* str_result = (char*)malloc(size + 1);
    char* tmp_result = str_result;
    tmp_start = start;
    for (int i = 0; i < size; i += 1) {
        *tmp_result = *tmp_start;
        tmp_result += 1;
        tmp_start += 1;
    }

    *tmp_result = 0;

    res.ptr = str_result;
    res.size = size;

    return(res);
}

// note: Triangle vertices are represented by 1-indexed (not 0-indexed) numbers, which correspond to the line
//       in the file in which they occur.
Polyhedron LoadOBJToMemory(FileReadResult mem, GameMemory* game_memory, PlatformServices services)
{
    Polyhedron new_model = {};
    // new_model.color = { 176, 40, 100, 255 };
    new_model.color = { 0xE0, 0xB0, 0xFF }; // mauve.
    char* ptr = (char*)mem.data;
    char* eof = ptr + mem.data_len;

    int current_line = 0;
    while (ptr != 0) {

        // Are first two characters vt?
        if (*ptr == 'v' && *(ptr + 1) == 't')  {
            ptr += 3;
            int counter = 1;
            v2f64 uv = {};
            while (counter < 3) {
                // read everything in until white space.
                char_size num = ReadAlphaNumericUntilWhiteSpace(ptr, eof);

                // if white space found, continue
                if (num.size == 0) {
                    ptr += 1;
                    continue;
                }

                // char* num_copy = strdup(num.ptr);
                char* str_end = num.ptr + num.size;
                float conversion_result = std::strtof(num.ptr, &str_end);

                // free(num_copy);

                // If we can't read a value as a number, log it. 
                if (str_end != num.ptr) {
                    if (counter == 1) {
                        uv.x = conversion_result;
                    } else if (counter == 2) {
                        uv.y = conversion_result;
                    }

                    counter += 1;
                    ptr += num.size;
                } else {
                    // Print line where problem occurred. 
                    printf("Invalid formatting on line %d.", current_line);
                }

                free(num.ptr);
            }

            AddToList<v2f64>(&new_model.uvs, uv, game_memory);
        }

        // Are first two characters vn?
        else if (*ptr == 'v' && *(ptr + 1) == 'n')  {
            ptr += 3;
            int counter = 1;
            v3f64 uv;
            while (counter < 4) {
                // read everything in until white space.
                char_size num = ReadAlphaNumericUntilWhiteSpace(ptr, eof);

                // if white space found, continue
                if (num.size == 0) {
                    ptr += 1;
                    continue;
                }

                char* str_end = num.ptr + num.size;
                float conversion_result = std::strtof(num.ptr, &str_end);

                if (str_end != num.ptr) {
                    if (counter == 1) {
                        uv.x = conversion_result;
                    } else if (counter == 2) {
                        uv.y = conversion_result;
                    } else if (counter == 3) {
                        uv.z = conversion_result;
                    }

                    counter += 1;
                    ptr += num.size;
                } else {
                    printf("Invalid formatting on line %d.", current_line);
                    break;
                }

                free(num.ptr);
            }

            AddToList<v3f64>(&new_model.normals, uv, game_memory);
        }

        // Is first character v?
        else if (*ptr == 'v')  {
            ptr += 2;
            int counter = 1;
            v3f64 uv;
            while (counter < 4) {
                // read everything in until white space.
                char_size num = ReadAlphaNumericUntilWhiteSpace(ptr, eof);

                // if white space found, continue
                if (num.size == 0) {
                    ptr += 1;
                    continue;
                }

                char* str_end = num.ptr + num.size;
                float conversion_result = std::strtof(num.ptr, &str_end);

                if (str_end != num.ptr) {
                    if (counter == 1) {
                        uv.x = conversion_result;
                    } else if (counter == 2) {
                        uv.y = conversion_result;
                    } else if (counter == 3) {
                        uv.z = conversion_result;
                    }

                    counter += 1;
                    ptr += num.size;
                } else {
                    printf("Invalid formatting on line %d.", current_line);
                    break;
                }

                free(num.ptr);
            }

            AddToList<v3f64>(&new_model.vertices, uv, game_memory);
        }

        // Is first character f?
        else if (*ptr == 'f') {
            ptr += 2;
            int counter = 1;
            TriangleRefVertices tri = {};
            while (counter < 4) {
                char_size num = ReadAlphaNumericUntilWhiteSpace(ptr, eof);

                // if white space found, continue
                if (num.size == 0) {
                    ptr += 1;
                    continue;
                }

                // Only a number and forward slash (/) present in a valid .OBJ.
                int forward_slash_counter = 0;
                int vert_num_counter = 1;
                char* vert_ptr = num.ptr;
                while (forward_slash_counter < 2) {
                    int index = std::atoi(vert_ptr);
                    if (index != 0) {
                        if (vert_num_counter == 1) {
                            tri.verts[counter - 1] = new_model.vertices.array + index - 1;
                        }
                        if (vert_num_counter == 2) {
                            tri.uvs[counter - 1] = new_model.uvs.array + index - 1;
                        }

                        vert_num_counter += 1;
                        vert_ptr += 1;
                    }
                    else {
                        forward_slash_counter += 1;
                        vert_ptr += 1;
                    }
                }

                // If next character is a number, this is the third and final number.
                int index = std::atoi(vert_ptr);
                if (index != 0) {
                    // study: Some kind of white space removal is occurring (sometimes) in which vert_ptr 
                    //       is pointing at two numbers, instead of a number followed by whitespace.
                    //       So, instead of '5 ', the ptr is at '58' which is now being read as an index of
                    //       '58' instead of just '5.
                    tri.normals[counter - 1] = new_model.normals.array + index - 1;

                    // if (counter == 1) {
                    //     tri.one_normal = normals.at(index - 1); 
                    // }
                    // if (counter == 2) {
                    //     tri.two_normal = normals.at(index - 1); 
                    // }
                    // if (counter == 3) {
                    //     tri.three_normal = normals.at(index - 1); 
                    // }
                } 

                ptr += num.size;
                counter += 1;
                free(num.ptr);
            }

            AddToList<TriangleRefVertices>(&new_model.triangles, tri, game_memory);
        }

        // Move ptr until line over!
        bool end_of_file = false;
        for (;;) {
            if (*ptr == '\n') {
                break;
            } else if (*ptr == 0) {
                end_of_file = true;
                break;
            }

            ptr += 1;
        }

        // Go to new line or we are done. 
        if (end_of_file == false) {
            ptr += 1; // Move to next line.
        } else {
            break;
        }

        current_line += 1;
    }

    // Free OBJ filedata, we won't be needing that.
    services.free_file_memory(mem.data);

    // game_memory->permanent_storage_remaining += mem.data_len;
    // game_memory->next_available -= mem.data_len;

    // We need to do a data copy for what we care about.
    // new_model.triangles = {};
    // AdjustMemory(sizeof(TriangleRefVertices), new_model.triangles.length, game_memory, 
    //         (void**)&(new_model.triangles.array));

    // memcpy(new_model.triangles.array, tris.data(), tris.size() * sizeof(TriangleRefVertices));

    // new_model.vertices = {};
    // AdjustMemory(sizeof(v3f64), verts.size(), game_memory, (void**)&(new_model.vertices.array));
    // new_model.vertices.length = verts.size();

    // memcpy(new_model.vertices.array, verts.data(), verts.size() * sizeof(v3f64));

    return(new_model);
}

static Area LoadArea(int id, GameState* game_state, GameMemory* game_memory, PlatformServices services, DoubleInterpolatedPattern* rumbles) 
{
    Area result = {};

    // Load models into memory.
    // study: how to load and store levels to and from memory.
    FileReadResult file_result = {};
    Polyhedron cube = {};

    services.read_file("cube.obj", &file_result);

    // services.read_file("building1.obj", &file_result);
    // services.read_file("building2.obj", &file_result);
    // services.read_file("building3.obj", &file_result);
    // services.read_file("elevator.obj", &file_result);
    // services.read_file("elevator.obj", &file_result);

    if (file_result.data) {
        cube = LoadOBJToMemory(file_result, game_memory, services);
    }

    // move the cube.
    cube.transform.pos = { 0, 0, 2 };

    // example of how to set rumble.
    // note: services.rumble_controller is expected to be called each frame.
    // f32 first_arr[3][2] = {{0, 0}, {20000, 29}, {0, 59}};
    // Sequence_f32 first = Sequence_f32::Create(first_arr, 3, game_memory);
    // Sequence_f32 second = Sequence_f32::Create(first_arr, 3, game_memory);
    // rumbles[0] = DoubleInterpolatedPattern::Create(first, second);
    // services.rumble_controller(0, rumbles);

    AddToList<Polyhedron>(&(result.friendlies), cube, game_memory);

    Stage(game_state, game_memory);

    return(result);
}
