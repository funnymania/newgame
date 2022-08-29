#ifndef AUDIO_H
#define AUDIO_H

struct StaticAudioStream 
{
    char name[32];
    IAudioClient* audio_client;
    IAudioRenderClient* render_client;
    REFERENCE_TIME buffer_duration;
    u32 buffer_frame_count; 
    BYTE* buffer;
    HANDLE wait_events[4]; // 0 is stop, 1 is pause, 2 is unpause
    bool paused;
    u32 action;
    u32 file_size;
    i32 data_remaining;
    u8* media_ptr;
};

#endif
