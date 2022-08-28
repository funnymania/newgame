#include "newgame.h"

static void RenderWeirdGradient(GameOffscreenBuffer *buffer, int x_offset, int y_offset) 
{
    int width = buffer->width;
    int height = buffer->height;

    int pitch = width * buffer->bytes_per_pixel;
    u8 *row = (u8 *)buffer->memory; 
    for (int y = 0; y < buffer->height; y += 1) {
        u8 *pixel = (u8 *)row;
        for (int x = 0; x < buffer->width; x += 1) {
            *pixel = (u8)(x - x_offset);
            ++pixel;

            *pixel = (u8)(y + y_offset);
            pixel += 1;

            *pixel = (u8)0;
            pixel += 1;

            *pixel = (u8)0;
            pixel += 1;
        }

        row += pitch;
    }
}

static void GameUpdateAndRender(GameOffscreenBuffer* buffer)
{
    SoundOutput();
    RenderWeirdGradient(buffer, buffer->x_offset, buffer->y_offset);
}

struct AudioPlayer {
    // Every meaningful OS definition comes here...
}

static void SoundOutput() 
{
    
}
