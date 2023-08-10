#include "display.h"
#include <stdint.h>
#include <math.h>

sprite_t frame = {0};
bool TEXTURED_AFFINE = true;

float* z_buffer = NULL;

void clear_color_buffer(uint32_t color)
{
    for (int y = 0; y < frame.h; y++)
        for (int x = 0; x < frame.w; x++)
        frame.pixels[y*frame.w + x] = color;
    
};
void draw_grid(void)
{
    for (int y = 0; y < frame.h; y+= 500)
        for(int x = 0 ; x < frame.w; x+= 500)
        frame.pixels[(y*frame.w + x) % (frame.w*frame.h)] = 0xFFFF0FFF;
};

void draw_pixel(int x, int y, uint32_t color)
{
    if (x >= 0 && x < frame.w && y >= 0 && y < frame.h)
        frame.pixels[y*frame.w + x] = color;
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color)
{
    int delta_x = (x1 - x0);
    int delta_y = (y1 - y0);
    
    int longest_side_length = (abs(delta_x) >= abs(delta_y)) ? abs(delta_x) : abs(delta_y);
    
    float x_inc = delta_x / (float)longest_side_length;
    float y_inc = delta_y / (float)longest_side_length;
    
    float current_x = x0;
    float current_y = y0;
    
    for (int i = 0; i <= longest_side_length; i++)
    {
        draw_pixel(round(current_x), round(current_y), color);
        current_x += x_inc;
        current_y += y_inc;
    }
}

void draw_rect(int x, int y, int width, int height, uint32_t color)
{
    width= (x+width > frame.w) ? x+width-frame.w: width;
    height= (y+height > frame.h) ? y+height-frame.h: height;
    
    for (int xi = x; xi < x+width; xi++)
    {
        for (int yi = y; yi < y+height; yi++)
            draw_pixel(xi, yi, color);
    }
}

void clear_z_buffer(void)
{
    for (int i = 0; i < frame.w*frame.h; i++)
        z_buffer[i] = 1.0;
}