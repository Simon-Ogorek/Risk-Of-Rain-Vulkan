#ifndef __GF3D_MESH_H__
#define __GF3D_MESH_H__

#define MESH_ATTRIBUTE_COUNT 5

#include <stdalign.h>
#include <vulkan/vulkan.h>

#include "gfc_vector.h"
#include "gfc_list.h"
#include "gfc_text.h"
#include "gfc_matrix.h"
#include "gfc_primitives.h"

#include "gf3d_pipeline.h"



//forward declaration:
typedef struct ObjData_S ObjData;

//absolute basics of the mesh information sent to the graphics card
typedef struct
{
    GFC_Matrix4     model;
    GFC_Matrix4     view;
    GFC_Matrix4     proj;

    GFC_Vector4D    color;
    GFC_Vector4D    camera;
    GFC_Vector4D    lightPos;
    GFC_Vector4D    lightColor;

    GFC_Matrix4     bones[64];

}MeshUBO;

typedef struct
{
    GFC_Matrix4     model;
    GFC_Matrix4     view;
    GFC_Matrix4     proj;
    GFC_Vector4D    color;
}SkyUBO;

typedef struct
{
    GFC_Vector3D vertex;
    GFC_Vector3D normal;
    GFC_Vector2D texel;
    GFC_Vector4D bones;
    GFC_Vector4D weights;
}Vertex;

typedef struct
{
    Uint16  verts[3];
}Face;

typedef struct
{
    Uint32          vertexCount;
    VkBuffer        vertexBuffer;
    VkDeviceMemory  vertexBufferMemory;
    Uint32          faceCount;
    VkBuffer        faceBuffer;
    VkDeviceMemory  faceBufferMemory;
    ObjData        *objData;
}MeshPrimitive;

typedef struct
{
    GFC_TextLine        filename;
    Uint32              _refCount;
    GFC_List           *primitives;
    GFC_Box             bounds;
}Mesh;

// Ripped from Sprite Manager
typedef struct
{
    Mesh            *mesh_list;      /**<pre-allocated space for sprites*/
    Uint32          max_meshes;      /**<maximum concurrent sprites supported*/
    Uint32          chain_length;     /**<length of swap chain*/
    VkDevice        device;           /**<logical vulkan device*/
    Pipeline       *pipe;             /**<the pipeline associated with sprite rendering*/
    Pipeline       *sky_pipe;
    VkBuffer        faceBuffer;       /**<memory handle for the face buffer (always two faces)*/
    VkDeviceMemory  faceBufferMemory; /**<memory habdle for tge face memory*/
    VkVertexInputAttributeDescription   attributeDescriptions[MESH_ATTRIBUTE_COUNT];
    VkVertexInputBindingDescription     bindingDescription;
    float           drawOrder;
    Texture         *defaultTexture;
} MeshManager;

extern MeshManager mesh_manager;
/**
 * @brief initializes the mesh system / configures internal data about mesh based rendering
 * @param mesh_max the maximum allowed simultaneous meshes supported at once.  Must be > 0
 * @note keep in mind that many models will be comprised of multiple sub meshes.  So this number may need to be very large
 */
void gf3d_mesh_init(Uint32 mesh_max);

/**
 * @brief get a new empty model
 * @return NULL on error, or an empty model
 */
Mesh *gf3d_mesh_new();

/**
 * @brief load mesh data from an obj filename.
 * @note: currently only supporting obj files
 * @note this free's the intermediate data loaded from the obj file, no longer needed for most applications
 * @param filename the name of the file to load
 * @return NULL on error or Mesh data
 */
Mesh *gf3d_mesh_load(const char *filename);

/**
 * @brief draw a mesh given the parameters
 */
void gf3d_mesh_draw(Mesh *mesh,GFC_Matrix4 modelMat,GFC_Color mod,Texture *texture,GFC_Vector3D lightPos,GFC_Color lightColor);
/**
 * @brief allocate a zero initialized mesh primitive
 * @return NULL on error or the primitive
 */

void gf3d_sky_draw(Mesh *mesh,GFC_Matrix4 modelMat,GFC_Color mod,Texture *texture);

MeshPrimitive *gf3d_mesh_primitive_new();


/**
 * @brief get the input attribute descriptions for mesh based rendering
 * @param count (optional, output) the number of attributes
 * @return a pointer to a vertex input attribute description array
 */
VkVertexInputAttributeDescription * gf3d_mesh_get_attribute_descriptions(Uint32 *count);

/**
 * @brief get the binding description for mesh based rendering
 * @return vertex input binding descriptions compatible with mesh data
 */
VkVertexInputBindingDescription * gf3d_mesh_get_bind_description();

/**
 * @brief free a mesh that has been loaded from memory
 */
void gf3d_mesh_free(Mesh *mesh);

/**
 * @brief create a mesh's internal buffers based on vertices
 * @param primitive the mesh primitive to populate
 * @note the primitive must have the objData set and it must have be organizes in buffer order
 */
void gf3d_mesh_create_vertex_buffer_from_vertices(MeshPrimitive *primitive);

void gf3d_mesh_setup_face_buffers(MeshPrimitive *prim);

/**
 * @brief get the pipeline that is used to render basic 3d meshes
 * @return NULL on error or the pipeline in question
 */
Pipeline *gf3d_mesh_get_pipeline();

/**
 * @brief given a model matrix and basic color, build the meshUBO needed to render a model
 * @param modelMat the model Matrix
 * @param colorMod the color for the UBO
 */
MeshUBO gf3d_mesh_get_ubo(GFC_Matrix4 modelMat, GFC_Color colorMod, GFC_Vector3D lightPos, GFC_Color lightColor);


#endif
