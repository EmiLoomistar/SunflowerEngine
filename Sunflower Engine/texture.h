#ifndef TEXTURE_H
#define TEXTURE_H

#include "display.h"
#include "upng.h"

typedef struct
{
    float u;
    float v;
} tex2_t;

void LoadSpritePNG(sprite_t* sprite, char* filename);
tex2_t tex2_clone(tex2_t* t);

#endif //TEXTURE_H
