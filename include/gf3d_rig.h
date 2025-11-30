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

struct Rigged_Mesh_S
{
    Mesh               *mesh;
    GLTF               *gltf;
    GFC_TextLine        filename;
    Uint32              _refCount;
};


void gf3d_rigged_init(Uint32 mesh_max);
Rigged_Mesh *gf3d_rigged_get_by_filename(const char *filename);

Rigged_Mesh *gf3d_rigged_new();
Rigged_Mesh *gf3d_rigged_load(const char *filename);


void gf3d_rigged_primitive_queue_render(MeshPrimitive *prim,Pipeline *pipe,void *uboData,Texture *texture);
void gf3d_rigged_queue_render(Rigged_Mesh *mesh,Pipeline *pipe,void *uboData,Texture *texture);

void gf3d_rigged_draw(Rigged_Mesh *mesh,GFC_Matrix4 modelMat,GFC_Color mod,Texture *texture,GFC_Vector3D lightPos,GFC_Color lightColor);


VkVertexInputAttributeDescription * gf3d_rigged_get_attribute_descriptions(Uint32 *count);
void gf3d_rigged_create_vertex_buffer_from_vertices(MeshPrimitive *primitive);
void gf3d_rigged_setup_face_buffers(MeshPrimitive *prim);
Pipeline *gf3d_rigged_get_pipeline();

void gf3d_rigged_free(Mesh *mesh);
#endif