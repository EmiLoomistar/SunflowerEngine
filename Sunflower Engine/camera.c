#include "camera.h"
#include "matrix.h"

static Camera_3d camera_3d;

void init_camera(vec3_t position, vec3_t direction)
{
    camera_3d.position = position;
    camera_3d.direction = direction;
    camera_3d.forward_velocity = vec3_new(0, 0, 0);
    camera_3d.yaw = 0.0;
    camera_3d.pitch = 0.0;
}

vec3_t get_camera_position(void)
{
    return camera_3d.position;
}
vec3_t get_camera_direction(void)
{
    return camera_3d.direction;
}
vec3_t get_camera_forward_velocity(void)
{
    return camera_3d.forward_velocity;
}
float get_camera_yaw(void)
{
    return camera_3d.yaw;
}
float get_camera_pitch(void)
{
    return camera_3d.pitch;
}
void update_camera_position(vec3_t position)
{
    camera_3d.position = position;
}
void update_camera_direction(vec3_t direction)
{
    camera_3d.direction = direction;
}
void update_camera_forward_velocity(vec3_t forward_velocity)
{
    camera_3d.forward_velocity = forward_velocity;
}
void rotate_camera_yaw(float angle)
{
    camera_3d.yaw += angle;
}
void rotate_camera_pitch(float angle)
{
    camera_3d.pitch += angle;
}

vec3_t get_camera_lookat_target(void)
{
    vec3_t target = {0, 0, 1}; // setting target
    mat4_t camera_yaw_rotation = mat4_make_rotation_y(camera_3d.yaw); // accounting for camera rotation locking y
    mat4_t camera_pitch_rotation = mat4_make_rotation_x(camera_3d.pitch); // accounting for camera rotation in locking x
    mat4_t camera_overall_rotation = mat4_mul_mat4(camera_yaw_rotation, camera_pitch_rotation);
    camera_3d.direction = vec3_from_vec4(mat4_mul_vec4(camera_overall_rotation, vec4_from_vec3(target))); 
    target = vec3_add(camera_3d.position, camera_3d.direction);
    return target;
}

camera_t camera = 
{
    .x = 0.0f,
    .y = 0.0f,
    .height = 14.0f,
    .horizon = 300.0f,
    .angle = -1*3.141592f/6*0,
    .zfar = 8192*4,
};