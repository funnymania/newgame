// Asks for memory appropriately circumstantially.
#include <stdio.h>
#include <string>

static DWORD bytes_read;

void PlatformFreeFileMemory(void* memory) 
{
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

void GameMemoryFree(void* obj_ptr, u32 obj_count, u32 obj_size) 
{
    u8* tmp = (u8*)obj_ptr;
    for (int counter = 0; counter < obj_size * obj_count; counter += 1) 
    {
        *tmp = 0;
    }
}

// todo: Second VirtualAlloc occurs here, we need to pull from game_memory->next_available.
void PlatformReadEntireFile(char* file_name, file_read_result* file_result, GameMemory* game_memory) 
{
    HANDLE file_handle = CreateFileA(file_name, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

    file_result->data = 0;

    if (file_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER file_size;
        if (GetFileSizeEx(file_handle, &file_size)) {
            // Assert(file_size.QuadPart <= 0xFFFFFFFF);
            u32 file_size_32 = (u32)file_size.QuadPart;
            // file_result->data = VirtualAlloc(0, file_size_32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE); 
            file_result->data = game_memory->next_available; 
            if (file_result->data) {
                if (ReadFile(file_handle, file_result->data, file_size_32, &bytes_read, 0) 
                        && file_size_32 == bytes_read) {
                    file_result->data_len = file_size_32;
                    game_memory->next_available += file_size_32;
                    game_memory->permanent_storage_remaining -= file_size_32;
                } else {
                    PlatformFreeFileMemory(file_result->data);
                }
            } else {
                //todo: logs.
            }
        } else {
            //todo: logs.
        }

        CloseHandle(file_handle);
    }
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
    while (std::isspace(*tmp_start) == 0 && tmp_start != eof) {
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
Obj* LoadOBJToMemory(char* file_name, GameMemory* game_memory)
{
    Obj* new_model = (Obj*)game_memory->next_available;
    game_memory->permanent_storage_remaining -= sizeof(Obj);
    game_memory->next_available += sizeof(Obj);

    file_read_result mem = {};

    PlatformReadEntireFile(file_name, &mem, game_memory);
    if (mem.data) {
        char* ptr = (char*)mem.data;
        char* eof = ptr + mem.data_len;
        std::vector<v3_float> verts;
        std::vector<v3_float> uvs;
        std::vector<v3_float> normals;
        std::vector<Tri> tris;
        int current_line = 0;
        while (ptr != 0) {

            // Are first two characters vt?
            if (*ptr == 'v' && *(ptr + 1) == 't')  {
                ptr += 3;
                int counter = 1;
                v3_float uv = {};
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

                uvs.push_back(uv);
            }

            // Are first two characters vn?
            else if (*ptr == 'v' && *(ptr + 1) == 'n')  {
                ptr += 3;
                int counter = 1;
                v3_float uv;
                while (counter < 4) {
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

                    // todo: error handling.
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

                normals.push_back(uv);
            }

            // Is first character v?
            else if (*ptr == 'v')  {
                ptr += 2;
                int counter = 1;
                v3_float uv;
                while (counter < 4) {
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

                    // todo: error handling.
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

                verts.push_back(uv);
            }

            // Is first character f?
            else if (*ptr == 'f') {
                ptr += 2;
                int counter = 1;
                Tri tri = {}; // Does this zero the vectors?
                while (counter < 4) {
                    // read Triangle Vertex in until white space.
                    if (current_line == 45) {
                        OutputDebugString("now");
                    }

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
                                tri.verts[counter - 1] = verts.at(index - 1);
                                // if (counter == 1) {
                                //     tri.one = verts.at(index - 1); 
                                // }
                                // if (counter == 2) {
                                //     tri.two = verts.at(index - 1); 
                                // }
                                // if (counter == 3) {
                                //     tri.three = verts.at(index - 1); 
                                // }
                            }
                            if (vert_num_counter == 2) {
                                // todo: UV.
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
                        tri.normals[counter - 1] = normals.at(index - 1);

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

                tris.push_back(tri);
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
        GameMemoryFree(mem.data, mem.data_len, 1);
        game_memory->permanent_storage_remaining += mem.data_len;
        game_memory->next_available -= mem.data_len;

        // We need to do a data copy from tri vector to triangles.
        new_model->triangles = (Tri*)game_memory->next_available;
        memcpy(new_model->triangles, tris.data(), tris.size() * sizeof(Tri));
        game_memory->permanent_storage_remaining -= tris.size() * sizeof(Tri);
        game_memory->next_available += tris.size() * sizeof(Tri);
        
        // new_model->triangles = tris.data();
        new_model->triangles_len = tris.size();

        // PlatformFreeFileMemory(mem.data);
    }

    return(new_model);
}
