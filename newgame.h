#ifndef NEWGAME_H
#define NEWGAME_H

// note: services that the game provides to the platform layer.

// Timer, Device input, bitmap buffer to use, sound buffer to use.
struct GameOffscreenBuffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int bytes_per_pixel = 4;
    int x_offset = 0; 
    int y_offset = 0; 
};

// note: this file contains types and functions that you implement differently ON EACH PLATFORM.

// there will be a Windowsy definition of AudioPlayer, there will be a 
// Windowsy definition of PlayAudio(). There will be an Androidy definition
// of AudioPlayer, etc etc.
// This means you end up trying to support unified abstractions for multiple different platforms.
// The abstraction 'AudioPlayer' might not make a lot of sense on Android, but since this is what 
// you use platform-independently, you end up having to work out the kinks.
struct AudioPlayer;

// 
void PlayAudio();

void GameUpdateAndRender(GameOffscreenBuffer* buffer);
void SoundOutput();


#endif
