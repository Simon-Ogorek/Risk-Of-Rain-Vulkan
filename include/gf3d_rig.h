#ifndef __GF3D_RIG_H__
#define __GF3D_RIG_H__

#include <stdalign.h>
#include <vulkan/vulkan.h>

#include "gfc_vector.h"
#include "gfc_list.h"
#include "gfc_text.h"
#include "gfc_matrix.h"
#include "gfc_primitives.h"

#include "gf3d_pipeline.h"
#include "gf3d_gltf_parse.h"
#include "gf3d_mesh.h"

typedef struct Rigged_Mesh_S Rigged_Mesh;

typedef struct GLTF_Node
{
    char* name;
    GFC_Matrix4 base_bone_matrix;
    GFC_Vector4D rotation;
    GFC_Vector3D scale;
    GFC_Vector3D translation;
    GFC_List *children;
    Uint8 root;
    int index;
    GFC_Matrix4 local_matrix;
    GFC_Matrix4 global_matrix;
    GFC_Matrix4 inverse_bind;
}GLTF_Node;

typedef enum AnimationType
{
    Translation,
    Rotation,
    Scale
}AnimationType;

typedef enum StructType // assuming all floats
{
    VEC2,
    VEC3,
    VEC4
}StructType;

typedef struct Sampler
{
    unsigned int count;    
    void * data;
    StructType type;

}Sampler;

typedef struct Channel
{
    Sampler* sample;
    GLTF_Node *target;
    AnimationType type;
}Channel;

typedef struct Animation
{
    char *name;
    GFC_List *channels;

    int frame_start;
    int frame_end;
    unsigned int frame_count;

    float seconds_start;
    float seconds_end;
    unsigned int seconds_duration;

    GFC_Matrix4 *inverse_bind_data;
    GFC_Matrix4 inverse_bind;

    int current_frame;
}Animation;

typedef struct 
{
    GFC_List *bones;
    Animation *animation;
    GFC_List *skin;
}Armature;

struct Rigged_Mesh_S
{
    Mesh               *mesh;
    GLTF               *gltf;
    GFC_TextLine        filename;
    Uint32              _refCount;
    Armature           *arm;
};


void gf3d_rigged_init(Uint32 mesh_max);
Rigged_Mesh *gf3d_rigged_get_by_filename(const char *filename);

Rigged_Mesh *gf3d_rigged_new();
Rigged_Mesh *gf3d_rigged_load(const char *filename);

void gf3d_rigged_parse_armarture(Rigged_Mesh* mesh);

void gf3d_rigged_primitive_queue_render(MeshPrimitive *prim,Pipeline *pipe,void *uboData,Texture *texture);
void gf3d_rigged_queue_render(Rigged_Mesh *mesh,Pipeline *pipe,void *uboData,Texture *texture);

void gf3d_rigged_draw(Rigged_Mesh *mesh,GFC_Matrix4 modelMat,GFC_Color mod,Texture *texture,GFC_Vector3D lightPos,GFC_Color lightColor);


VkVertexInputAttributeDescription * gf3d_rigged_get_attribute_descriptions(Uint32 *count);
void gf3d_rigged_create_vertex_buffer_from_vertices(MeshPrimitive *primitive);
void gf3d_rigged_setup_face_buffers(MeshPrimitive *prim);
Pipeline *gf3d_rigged_get_pipeline();

void gf3d_rigged_free(Mesh *mesh);

#endif