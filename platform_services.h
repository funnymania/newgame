#ifndef PLATFORM_SERVICES_H
#define PLATFORM_SERVICES_H

struct PlatformServices
{
    load_obj* load_obj;    
    persona_handshake* persona_handshake;    
    load_sound* load_sound;    
    start_playing* start_playing;    
    pause_audio_game* pause_audio;
    resume_audio_game* resume_audio;
};

#endif
