#ifndef ARRAY_H
#define ARRAY_H
#include <stdlib.h>
#include "triangle.h"
#include "vector.h"

/* example snippet:
      Array array;
    triangle_t triangle = {0};
    initArray(&array);
    writeArray(&array, array);
    freeArray(&array);*/

// Dynamic Array of triangles (totally dynamic!)
typedef struct
{
    int count;
    int capacity;
    triangle_t* triangles;
} ArrayT;
void initArrayT(ArrayT* array);
void writeArrayT(ArrayT* array, triangle_t triangle);
void freeArrayT(ArrayT* array);
// Dynamic Array of vertices (for loading .obj meshes)
typedef struct
{
    int count;
    int capacity;
    vec3_t* vertices;
} ArrayV;
void initArrayV(ArrayV* array);
void writeArrayV(ArrayV* array, vec3_t vector);
void freeArrayV(ArrayV* array);
// Dynamic Array of faces (for loading .obj meshes)
typedef struct
{
    int count;
    int capacity;
    face_t* faces;
} ArrayF;
void initArrayF(ArrayF* array);
void writeArrayF(ArrayF* array, face_t face);
void freeArrayF(ArrayF* array);
typedef struct
{
    int count;
    int capacity;
    tex2_t* textures;
} ArrayTextures;
void writeArrayTextures(ArrayTextures* array, tex2_t texture);
void freeArrayTextures(ArrayTextures* array);

#endif //ARRAYH