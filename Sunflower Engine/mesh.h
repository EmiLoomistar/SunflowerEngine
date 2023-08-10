#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "triangle.h"
#include "array.h"
#include "upng.h"

///////////////////////////////////////////////////////
// Define a struct for dynamic size meshed
//////////////////////////////////////////////////////
typedef struct
{
    ArrayV arrayV;    // dynamic array of vertices
    ArrayF arrayF;    // dynamic array of faces
    upng_t* texture;  // pointer to UPNG
    vec3_t rotation;  // vector specifing rotation
    vec3_t scale;     // scale with x, y and z values
    vec3_t translation;
} mesh_t;

void load_mesh(char* obj_filename, char* png_filename, vec3_t scale, vec3_t translation, vec3_t rotation);
void load_mesh_obj_data(mesh_t* mesh, char* obj_filename);
void load_mesh_png_data(mesh_t* mesh, char* png_filename);

int get_num_meshes(void);
mesh_t* get_mesh(int index);

void free_meshes(void);

#endif //MESH_H