#include "vector.h"
#include <math.h>
#include <stdint.h>

float Q_rsqrt(float number)
{
    int32_t i;
    float x2, y;
    const float threehalfs = 1.5F;
    
    x2 = number * 0.5F;
    y = number;
    i = *(uint32_t*)&y;                // evil floating point bit hack
    i = 0x5f3759df - (i >> 1);         // What the fuck
    y = *(float*)&i;
    y = y*(threehalfs-(x2*y*y));       // 1st iteration
    // y = y * (threehalfs - (x2*y*y));// 2nd iteration (can be removed)
    return y;
}

/* Vector 2D functions */

vec2_t vec2_new(float x, float y)
{
    vec2_t result = {x, y};
    return result;
}

float vec2_length(vec2_t v) 
{
    return sqrt(v.x * v.x + v.y * v.y);
}

vec2_t vec2_add(vec2_t a, vec2_t b) 
{
    vec2_t result = {
        .x = a.x + b.x,
        .y = a.y + b.y
    };
    return result;
}

vec2_t vec2_sub(vec2_t a, vec2_t b) 
{
    vec2_t result = {
        .x = a.x - b.x,
        .y = a.y - b.y
    };
    return result;
}

vec2_t vec2_mul(vec2_t v, float factor) 
{
    vec2_t result = {
        .x = v.x * factor,
        .y = v.y * factor
    };
    return result;
}

vec2_t vec2_div(vec2_t v, float factor) 
{
    vec2_t result = {
        .x = v.x / factor,
        .y = v.y / factor
    };
    return result;
}

float vec2_dot(vec2_t a, vec2_t b) 
{
    return (a.x * b.x) + (a.y * b.y);
}

void vec2_normalize(vec2_t* v)
{
    
    float length = sqrt(v->x * v->x + v->y * v->y);
    v->x /= length; 
    v->y /= length; 
}

void vec2_Qnormalize(vec2_t* v)
{
    
    float factor = Q_rsqrt(v->x * v->x + v->y * v->y);
    v->x *= factor;
    v->y *= factor; 
}
/* Implementations of Vector 3D functions */

vec3_t vec3_new(float x, float y, float z)
{
    vec3_t result = {x, y, z};
    return result;
}

vec3_t vec3_clone(vec3_t* v)
{
    vec3_t result = {v->x, v->y, v->z};
    return result;
}

float vec3_length(vec3_t v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3_t vec3_add(vec3_t a, vec3_t b) {
    vec3_t result = {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z
    };
    return result;
}

vec3_t vec3_sub(vec3_t a, vec3_t b) {
    vec3_t result = {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z
    };
    return result;
}

vec3_t vec3_mul(vec3_t v, float factor) {
    vec3_t result = {
        .x = v.x * factor,
        .y = v.y * factor,
        .z = v.z * factor
    };
    return result;
}

vec3_t vec3_div(vec3_t v, float factor) {
    vec3_t result = {
        .x = v.x / factor,
        .y = v.y / factor,
        .z = v.z / factor
    };
    return result;
}

vec3_t vec3_cross(vec3_t a, vec3_t b) {
    vec3_t result = {
        .x = a.y * b.z - a.z * b.y,
        .y = a.z * b.x - a.x * b.z,
        .z = a.x * b.y - a.y * b.x
    };
    return result;
}

float vec3_dot(vec3_t a, vec3_t b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

void vec3_normalize(vec3_t* v)
{
    float length = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x /= length; 
    v->y /= length; 
    v->z /= length; 
}

void vec3_Qnormalize(vec3_t* v)
{
    float factor = Q_rsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x *= factor; 
    v->y *= factor; 
    v->z *= factor; 
}
vec3_t vec3_rotate_x(vec3_t v, float angle) {
    vec3_t rotated_vector = {
        .x = v.x,
        .y = v.y * cos(angle) - v.z * sin(angle),
        .z = v.y * sin(angle) + v.z * cos(angle)
    };
    return rotated_vector;
}

vec3_t vec3_rotate_y(vec3_t v, float angle) {
    vec3_t rotated_vector = {
        .x = v.x * cos(angle) - v.z * sin(angle),
        .y = v.y,
        .z = v.x * sin(angle) + v.z * cos(angle)
    };
    return rotated_vector;
}

vec3_t vec3_rotate_z(vec3_t v, float angle) {
    vec3_t rotated_vector = {
        .x = v.x * cos(angle) - v.y * sin(angle),
        .y = v.x * sin(angle) + v.y * cos(angle),
        .z = v.z
    };
    return rotated_vector;
}

/* Implementations of Vector conversion functions */
vec4_t vec4_from_vec3(vec3_t v)
{
    vec4_t result = {v.x, v.y, v.z, 1.0};
    return result;
}
vec3_t vec3_from_vec4(vec4_t v)
{
    vec3_t result = {v.x, v.y, v.z};
    return result;
}

vec2_t vec2_from_vec4(vec4_t v)
{
    vec2_t result = {v.x, v.y};
    return result;
}

vec2_t vec2_from_vec3(vec3_t v)
{
    vec2_t result = {v.x, v.y};
    return result;
}