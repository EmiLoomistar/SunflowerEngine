#ifndef CAMERA_H
#define CAMERA_H

#include "vector.h"

typedef struct
{
    vec3_t position; // x, y, z
    vec3_t direction; // where camera is looking at
    vec3_t forward_velocity; // fps camera movement (forwards & backwards)
    float yaw; // same as angle
    float pitch; // same as horizon
} Camera_3d;


void init_camera(vec3_t position, vec3_t direction);

vec3_t get_camera_position(void);
vec3_t get_camera_direction(void);
vec3_t get_camera_forward_velocity(void);
float get_camera_yaw(void);
float get_camera_pitch(void);

void update_camera_position(vec3_t position);
void update_camera_direction(vec3_t direction);
void update_camera_forward_velocity(vec3_t forward_velocity);

void rotate_camera_yaw(float angle);
void rotate_camera_pitch(float angle);

vec3_t get_camera_lookat_target(void);

typedef struct
{
    float x;
    float y;
    float height;
    float angle;
    float horizon; // looking up-down
    float zfar;
} camera_t;
extern camera_t camera;

#endif //CAMERA_H
