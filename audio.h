#ifndef AUDIO_H
#define AUDIO_H

struct StaticAudioStream 
{
    IAudioClient* audio_client;
    IAudioRenderClient* render_client;
    REFERENCE_TIME buffer_duration;
    u32 buffer_frame_count; 
    BYTE* buffer;
    HANDLE wait_events[3]; // 0 is stop, 1 is pause, 2 is unpause
    bool paused;
    u32 action;
    i32 data_remaining;
};

#endif