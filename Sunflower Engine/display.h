#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

enum cull_method
{
    CULL_NONE,
    CULL_BACKFACE,
} cull_method;

extern bool TEXTURED_AFFINE;

enum render_method
{
    RENDER_WIRE,
    RENDER_WIRE_VERTEX,
    RENDER_FILL_TRIANGLE,
    RENDER_FILL_TRIANGLE_WIRE,
    RENDER_TEXTURED,
    RENDER_TEXTURED_WIRE,
} render_method;

typedef struct {
    union {int width, w;};
    union {int height, h;};
    union {uint32_t *pixels, *p;};
} sprite_t;
extern sprite_t frame; //filled in WM_SIZE btw
extern float* z_buffer;

void clear_color_buffer(uint32_t color);
void draw_grid(void);
void draw_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
void clear_z_buffer(void);
#endif //DISPLAY_H

