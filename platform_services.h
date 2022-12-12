#ifndef PLATFORM_SERVICES_H
#define PLATFORM_SERVICES_H

typedef void read_file(char* file_name, FileReadResult* file_result);
typedef void free_file_memory(void* mem);

struct PlatformServices
{
    read_file* read_file;
    free_file_memory* free_file_memory;
    rumble_controller* rumble_controller;    
    load_sound* load_sound;    
    start_playing* start_playing;    
    pause_audio_game* pause_audio;
    resume_audio_game* resume_audio;
};

#endif
