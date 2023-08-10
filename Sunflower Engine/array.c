#include "array.h"

void initArrayT(ArrayT* array)
{
    array->count = 0;
    array->capacity = 0;
    array->triangles = NULL;
}
void freeArrayT(ArrayT* array)
{
    free(array->triangles);
    initArrayT(array);
}
void writeArrayT(ArrayT* array, triangle_t triangle)
{
    if (array->capacity < array->count + 1)
    {
        array->capacity = (array->capacity<512) ? 512 : array->capacity*2;
        array->triangles = (triangle_t*)realloc(array->triangles, sizeof(triangle_t)*array->capacity);
    }
    array->triangles[array->count] = triangle;
    array->count++;
}

void initArrayV(ArrayV* array)
{
    array->count = 0;
    array->capacity = 0;
    array->vertices = NULL;
}
void freeArrayV(ArrayV* array)
{
    free(array->vertices);
    initArrayV(array);
}
void writeArrayV(ArrayV* array, vec3_t vector)
{
    if (array->capacity < array->count + 1)
    {
        array->capacity = (array->capacity<8) ? 8 : array->capacity*2;
        array->vertices = (vec3_t*)realloc(array->vertices, sizeof(vec3_t)*array->capacity);
    }
    array->vertices[array->count] = vector;
    array->count++;
}

void initArrayF(ArrayF* array)
{
    array->count = 0;
    array->capacity = 0;
    array->faces = NULL;
}
void freeArrayF(ArrayF* array)
{
    free(array->faces);
    initArrayF(array);
}
void writeArrayF(ArrayF* array, face_t face)
{
    if (array->capacity < array->count + 1)
    {
        array->capacity = (array->capacity<8) ? 8 : array->capacity*2;
        array->faces = (face_t*)realloc(array->faces, sizeof(face_t)*array->capacity);
    }
    array->faces[array->count] = face;
    array->count++;
}

void initArrayTextures(ArrayTextures* array)
{
    array->count = 0;
    array->capacity = 0;
    array->textures = NULL;
}
void freeArrayTextures(ArrayTextures* array)
{
    free(array->textures);
    initArrayTextures(array);
}
void writeArrayTextures(ArrayTextures* array, tex2_t texture)
{
    if (array->capacity < array->count + 1)
    {
        array->capacity = (array->capacity<8) ? 8 : array->capacity*2;
        array->textures = (tex2_t*)realloc(array->textures, sizeof(tex2_t)*array->capacity);
    }
    array->textures[array->count] = texture;
    array->count++;
}
