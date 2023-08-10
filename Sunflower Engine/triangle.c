#include "display.h"
#include "triangle.h"
#include "swap.h"
#include <stdlib.h>

void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color)
{
    draw_line(x0, y0, x1, y1, color);
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x0, y0, color);
}

///////////////////////////////////////////////////////////////////////////////
// Return the barycentric weights alpha, beta, and gamma for point p
///////////////////////////////////////////////////////////////////////////////
//
//         (B)
//         /|\
//        / | \
//       /  |  \
//      /  (P)  \
//     /  /   \  \
//    / /       \ \
//   //           \\
//  (A)------------(C)
//
///////////////////////////////////////////////////////////////////////////////
vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p) {
    // Find the vectors between the vertices ABC and point p
    vec2_t ac = vec2_sub(c, a);
    vec2_t ab = vec2_sub(b, a);
    vec2_t ap = vec2_sub(p, a);
    vec2_t pc = vec2_sub(c, p);
    vec2_t pb = vec2_sub(b, p);
    
    // Compute the area of the full parallegram/triangle ABC using 2D cross product
    float area_parallelogram_abc = (ac.x * ab.y - ac.y * ab.x); // || AC x AB ||
    
    // Alpha is the area of the small parallelogram/triangle PBC divided by the area of the full parallelogram/triangle ABC
    float alpha = (pc.x * pb.y - pc.y * pb.x) / area_parallelogram_abc;
    
    // Beta is the area of the small parallelogram/triangle APC divided by the area of the full parallelogram/triangle ABC
    float beta = (ac.x * ap.y - ac.y * ap.x) / area_parallelogram_abc;
    
    // Weight gamma is easily found since barycentric coordinates always add up to 1.0
    float gamma = 1 - alpha - beta;
    
    vec3_t weights = { alpha, beta, gamma };
    return weights;
}

/////////////////////////////////////////////////////////////////////////////////
// Function to draw texel at x and y with interpolation
void draw_texel(int x, int y, upng_t* texture, // current point
                vec4_t point_a, vec4_t point_b, vec4_t point_c,
                tex2_t a_uv, tex2_t b_uv, tex2_t c_uv)// overall triangle vectors (2 for 2d, 4 for affine)
//float u0, float v0, float u1, float v1, float u2, float v2) // text sample
{ 
    vec2_t p = { x, y }; // current point
    vec2_t a = vec2_from_vec4(point_a);
    vec2_t b = vec2_from_vec4(point_b);
    vec2_t c = vec2_from_vec4(point_c);
    
    vec3_t weights = barycentric_weights(a, b, c, p); // barycentric with respect to p
    
    float alpha = weights.x;
    float beta = weights.y;
    float gamma = weights.z;
    
    float interpolated_u;
    float interpolated_v;
    float interpolated_reciprocal_w; // Pikuma's explanation
    
    if (TEXTURED_AFFINE)
    {
#if 1
        // interpolation of all U/w and V/w values 
        interpolated_u = (a_uv.u / point_a.w)*alpha + (b_uv.u / point_b.w)*beta + (c_uv.u/point_c.w)*gamma;
        interpolated_v = (a_uv.v / point_a.w)*alpha + (b_uv.v / point_b.w)*beta + (c_uv.v/point_c.w)*gamma;
        // also interpolating the value of 1/w for current pixel
        interpolated_reciprocal_w = (1/point_a.w)*alpha + (1/point_b.w)*beta + (1/point_c.w)*gamma;
        // divide back interpolated values by 1/w
        interpolated_u /= interpolated_reciprocal_w;
        interpolated_v /= interpolated_reciprocal_w;
#else
        float A = alpha*point_b.w*point_c.w;
        float B = beta*point_a.w*point_c.w;
        float C = gamma*point_a.w*point_b.w;
        float reciprocal = A+B+C;
        interpolated_u = (a_uv.u*A+b_uv.u*B+c_uv.u*C)/reciprocal;
        interpolated_v = (a_uv.v*A+b_uv.v*B+c_uv.v*C)/reciprocal;
#endif
    }
    else
    {
        interpolated_u = a_uv.u*alpha + b_uv.u*beta + c_uv.u*gamma;// getting interpolated coordinate from uv triangle
        interpolated_v = a_uv.v*alpha + b_uv.v*beta + c_uv.v*gamma;// sampling texel coordinate (between 0-1)
    }
    
    // Get the mesh texture width and height
    int texture_width = upng_get_width(texture);
    int texture_height = upng_get_height(texture);
    
    int tex_x = abs((int)(interpolated_u * texture_width)) % texture_width; //scalating to texture dimensions (now between 0-64)
    int tex_y = abs((int)(interpolated_v * texture_height)) % texture_height;
    
    //draw_pixel(x, y, texture[texture_width*tex_y + tex_x]);
    int idx = y*frame.w+x;
    interpolated_reciprocal_w = 1.0 - interpolated_reciprocal_w; // accounting for fact that farther are smaller
    if (interpolated_reciprocal_w < z_buffer[idx] )
    {
        uint32_t* texture_buffer = (uint32_t*)upng_get_buffer(texture);
        uint32_t color = texture_buffer[texture_width*tex_y+tex_x];
        
        frame.pixels[idx] = color;
        z_buffer[idx] =  interpolated_reciprocal_w;
        /*
		frame.pixels[idx+1] = color;
        frame.pixels[idx+2] = color;
        frame.pixels[idx+3] = color;
        //There must be a better approach//
        frame.pixels[idx+4] = color;
        frame.pixels[idx+5] = color;
        frame.pixels[idx+6] = color;
        frame.pixels[idx+7] = color;
        
        z_buffer[idx+1] = interpolated_reciprocal_w;
        z_buffer[idx+2] = interpolated_reciprocal_w;
        z_buffer[idx+3] = interpolated_reciprocal_w;*/
    }
    
}

///////////////////////////////////////////////////////////////////////////////
// Draw a textured triangle based on a texture array of colors.
// We split the original triangle in two, half flat-bottom and half flat-top.
///////////////////////////////////////////////////////////////////////////////
//
//        v0
//        /\
//       /  \
//      /    \
//     /      \
//   v1--------\
//     \_       \
//        \_     \
//           \_   \
//              \_ \
//                 \\
//                   \
//                    v2
//
///////////////////////////////////////////////////////////////////////////////
void draw_textured_triangle(int x0, int y0, float z0, float w0, float u0, float v0,
                            int x1, int y1, float z1, float w1, float u1, float v1,
                            int x2, int y2, float z2, float w2, float u2, float v2, upng_t* texture)
{
    // We need to sort the vertices by y-coordinate ascending (y0 < y1 < y2)
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }
    if (y1 > y2) {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&u1, &u2);
        float_swap(&v1, &v2);
        float_swap(&z1, &z2);
        float_swap(&w1, &w2);
    }
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }
    
    //flip v to account for inverted UV in rendering phase
    v0 = 1 - v0;
    v1 = 1 - v1;
    v2 = 1 - v2;
    
    // Create vector points after we sort the vertices
    vec4_t point_a = {x0, y0, z0, w0};
    vec4_t point_b = {x1, y1, z1, w1};
    vec4_t point_c = {x2, y2, z2, w2};
    tex2_t a_uv = {u0, v0};
    tex2_t b_uv = {u1, v1};
    tex2_t c_uv = {u2, v2};
    
    /* Rendering lower part */
    float inv_slope_1 = 0;
    float inv_slope_2 = 0;
    
    if ((y1-y0) != 0) inv_slope_1 = (float)(x1-x0)/abs(y1-y0); // change in x1 with respect to y
    if ((y2-y0) != 0) inv_slope_2 = (float)(x2-x0)/abs(y2-y0); // change in x2 with respect to y
    
    if (y1-y0 != 0){
        for (int y = y0; y <= y1; y++)
        {
            int x_end   = x0 + (y - y0) * inv_slope_2; // get change in x2 and add to x0
            int x_start = x1 + (y - y1) * inv_slope_1; // ?? why this not x0 too?
            
            if (x_end < x_start) int_swap(&x_end, &x_start);
            
            for (int x = x_start; x < x_end; x++)
            { // Performing a direct pattern texturing
                if ((y*frame.w+x >= frame.h*frame.w-7) || (y*frame.w+x < 0)) break;
                draw_texel(x, y, texture, point_a, point_b, point_c,a_uv, b_uv, c_uv );
            }
        }
    }
    
    /*Rendering upper part*/
    inv_slope_1 = 0;
    inv_slope_2 = 0;
    
    if ((y2-y1) != 0) inv_slope_1 = (float)(x2-x1)/abs(y2-y1); // change in x1 with respect to y
    if ((y2-y0) != 0) inv_slope_2 = (float)(x2-x0)/abs(y2-y0); // change in x2 with respect to y
    
    if (y2-y1 != 0)
    {
        for (int y = y1; y <= y2; y++)
        {
            int x_end   = x0 + (y - y0) * inv_slope_2; // get change in x2 and add to x0
            int x_start = x1 + (y - y1) * inv_slope_1; // ?? shouldnt it be x0 too?
            
            if (x_end < x_start) int_swap(&x_end, &x_start);
            
            for (int x = x_start; x < x_end; x++)
            {
                if ((y*frame.w+x >= frame.h*frame.w-7) || (y*frame.w+x < 0)) break;
                draw_texel(x, y, texture, point_a, point_b, point_c, a_uv, b_uv, c_uv);
                
            }
        }
    }
}


void draw_triangle_pixel(
                         int x, int y, uint32_t color,
                         vec4_t point_a, vec4_t point_b, vec4_t point_c
                         ) {
    // Create three vec2 to find the interpolation
    vec2_t p = { x, y };
    vec2_t a = vec2_from_vec4(point_a);
    vec2_t b = vec2_from_vec4(point_b);
    vec2_t c = vec2_from_vec4(point_c);
    
    // Calculate the barycentric coordinates of our point 'p' inside the triangle
    vec3_t weights = barycentric_weights(a, b, c, p);
    
    float alpha = weights.x;
    float beta = weights.y;
    float gamma = weights.z;
    
    float interpolated_reciprocal_w = (1 / point_a.w) * alpha + (1 / point_b.w) * beta + (1 / point_c.w) * gamma;
    interpolated_reciprocal_w = 1.0 - interpolated_reciprocal_w; // acounting for fact that farther are smaller
    
    int idx = y*frame.w+x;
    if (interpolated_reciprocal_w < z_buffer[idx])
    {
        frame.pixels[idx] = color;
        frame.pixels[idx+1] = color;
        frame.pixels[idx+2] = color;
        
        z_buffer[idx] =  interpolated_reciprocal_w;
        z_buffer[idx+1] = interpolated_reciprocal_w;
        
    }
    
}
///////////////////////////////////////////////////////////////////////////////
// Draw a filled triangle with the flat-top/flat-bottom method
// We split the original triangle in two, half flat-bottom and half flat-top
///////////////////////////////////////////////////////////////////////////////
//
//          (x0,y0)
//            / \
//           /   \
//          /     \
//         /       \
//        /         \
//   (x1,y1)------(Mx,My)
//       \_           \
//          \_         \
//             \_       \
//                \_     \
//                   \    \
//                     \_  \
//                        \_\
//                           \
//                         (x2,y2)
//
///////////////////////////////////////////////////////////////////////////////
void draw_filled_triangle(int x0, int y0, float z0, float w0,
                          int x1, int y1, float z1, float w1, 
                          int x2, int y2, float z2, float w2, uint32_t color) {
    // We need to sort the vertices by y-coordinate ascending (y0 < y1 < y2)
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }
    if (y1 > y2) {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&z1, &z2);
        float_swap(&w1, &w2);
    }
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }
    
    vec4_t point_a = { x0, y0, z0, w0};
    vec4_t point_b = { x1, y1, z1, w1};
    vec4_t point_c = { x2, y2, z2, w2};
    
    ///////////////////////////////////////////////////////
    // Render the upper part of the triangle (flat-bottom)
    ///////////////////////////////////////////////////////
    float inv_slope_1 = 0;
    float inv_slope_2 = 0;
    
    if (y1 - y0 != 0) inv_slope_1 = (float)(x1 - x0) / abs(y1 - y0);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);
    
    if (y1 - y0 != 0) {
        for (int y = y0; y <= y1; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;
            
            if (x_end < x_start) 
                int_swap(&x_start, &x_end); // swap if x_start is to the right of x_end
            
            for (int x = x_start; x < x_end; x+=2) {
                // Draw our pixel with a solid color
                if ((y*frame.w+x >= frame.h*frame.w-2) || (y*frame.w+x < 0)) break;
                draw_triangle_pixel(x, y, color, point_a, point_b, point_c);
            }
        }
    }
    
    ///////////////////////////////////////////////////////
    // Render the bottom part of the triangle (flat-top)
    ///////////////////////////////////////////////////////
    inv_slope_1 = 0;
    inv_slope_2 = 0;
    
    if (y2 - y1 != 0) inv_slope_1 = (float)(x2 - x1) / abs(y2 - y1);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);
    
    if (y2 - y1 != 0) {
        for (int y = y1; y <= y2; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;
            
            if (x_end < x_start) 
                int_swap(&x_start, &x_end); // swap if x_start is to the right of x_end
            
            for (int x = x_start; x < x_end; x+=2) {
                // Draw our pixel with a solid color
                if ((y*frame.w+x >= frame.h*frame.w-2) || (y*frame.w+x < 0)) break;
                draw_triangle_pixel(x, y, color, point_a, point_b, point_c);
            }
        }
    }
    
}
