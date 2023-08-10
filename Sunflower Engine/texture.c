#include <stdio.h>
#include "texture.h"

void LoadSpritePNG(sprite_t* sprite, char* filename)
{
    upng_t* upng;
    upng = upng_new_from_file(filename);
    if (upng != NULL) {
        upng_decode(upng);
        if (upng_get_error(upng) == UPNG_EOK) {
            // stuff
            sprite->width = upng_get_width(upng);
            sprite->height = upng_get_height(upng);
            sprite->pixels = (uint32_t*)upng_get_buffer(upng);
            
            if (upng_get_format(upng) == UPNG_RGBA8)
            {
                uint32_t* temp = (uint32_t*)upng_get_buffer(upng);
                uint32_t r, g, b, a;
                for (int i = 0; i < sprite->width*sprite->height; i++)
                {
                    r = temp[i]&0xFF000000;
                    g = temp[i]&0x00FF0000;
                    b = temp[i]&0x0000FF00;
                    a = temp[i]&0x000000FF;
                    sprite->pixels[i] =  (r>>0) | (a<<16) | (b>>0) | (g>>16);
                }
            }
        }
    }
    //upng_free(upng);
}

tex2_t tex2_clone(tex2_t* t)
{
    tex2_t result = {t->u, t->v};
    return result;
}