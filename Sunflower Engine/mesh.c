#include "mesh.h"
#include "array.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_NUM_MESHES 10
static mesh_t meshes[MAX_NUM_MESHES];
static int mesh_count = 0;

void load_mesh_obj_data(mesh_t* mesh, char* obj_filename)
{
    char s[512]; // string buffer
    FILE* file;
    fopen_s(&file, obj_filename, "r");
    
    ArrayTextures texcoords;
    initArrayTextures(&texcoords);
    
    while (fgets(s, sizeof(s), file) != NULL)
    {
        if (s[0] == 'v' && s[1] == ' ') // if found a vector
        {
            float v1, v2, v3;
            sscanf(s, "v %f %f %f", &v1, &v2, &v3);
            vec3_t cube_vertex = {.x = v1*1, .y = v2*1, .z = v3*1};
            writeArrayV(&mesh->arrayV, cube_vertex);
        }
        if (s[0] == 'v' && s[1] == 't')
        {
            tex2_t texcoord;
            sscanf(s, "vt %f %f", &texcoord.u, &texcoord.v);
            writeArrayTextures(&texcoords, texcoord);
        }
        else if (s[0] == 'f') // found a face
        {
            int v1, v2, v3;
            int vt1, vt2, vt3;
            int vn1, vn2, vn3;
            sscanf(s, "f %d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
            face_t cube_face = {
                .a = v1 - 1, 
                .b = v2 - 1, 
                .c = v3 - 1, 
                .a_uv = texcoords.textures[vt1-1],
                .b_uv = texcoords.textures[vt2-1],
                .c_uv = texcoords.textures[vt3-1],
                .color = 0xFFFFFFFF};
            writeArrayF(&mesh->arrayF, cube_face);
        }
    }
    freeArrayTextures(&texcoords);
    fclose(file);
}


void load_mesh_png_data(mesh_t* mesh, char* png_filename)
{
    upng_t* png_image = upng_new_from_file(png_filename);
    if (png_image != NULL) {
        upng_decode(png_image);
        if (upng_get_error(png_image) == UPNG_EOK) {
            if (upng_get_format(png_image) == UPNG_RGBA8)
            {
                // stuff in case image in wrong windows format
                uint32_t* mesh_texture = (uint32_t*)upng_get_buffer(png_image);
                int texture_width = upng_get_width(png_image);
                int texture_height = upng_get_height(png_image);
                
                uint32_t* temp = (uint32_t*)upng_get_buffer(png_image);
                uint32_t r, g, b, a;
                for (int i = 0; i < texture_width*texture_height; i++)
                {
                    r = temp[i]&0xFF000000;
                    g = temp[i]&0x00FF0000;
                    b = temp[i]&0x0000FF00;
                    a = temp[i]&0x000000FF;
                    mesh_texture[i] =  (r>>0) | (a<<16) | (b>>0) | (g>>16);
                }
            }
            // if we could decode image
            mesh->texture = png_image;
        }
    }
}

void load_mesh(char* obj_filename, char* png_filename, vec3_t scale, vec3_t translation, vec3_t rotation)
{
    load_mesh_obj_data(&meshes[mesh_count], obj_filename);
    load_mesh_png_data(&meshes[mesh_count], png_filename);
    
    meshes[mesh_count].scale = scale;
    meshes[mesh_count].translation = translation;
    meshes[mesh_count].rotation = rotation;
    
    mesh_count++;
}

int get_num_meshes(void)
{
    return mesh_count;
}


mesh_t* get_mesh(int index)
{
    return &meshes[index];
}

void free_meshes(void)
{
    for (int i = 0; i < mesh_count; i++)
    {
        upng_free(meshes[i].texture);
        freeArrayF(&meshes[i].arrayF);
        freeArrayV(&meshes[i].arrayV);
    }
}