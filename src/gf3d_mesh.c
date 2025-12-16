#include "simple_logger.h"
#include "gf3d_obj_load.h"
#include "gf3d_mesh.h"
#include "gf3d_buffers.h"
#include "gf3d_swapchain.h"
#include "gf3d_camera.h"
#include "gf3d_vgraphics.h"

MeshManager mesh_manager;

void gf3d_mesh_init(Uint32 mesh_max)
{
    Uint32 count;

    count = 0;
    if (mesh_max == 0)
    {
        slog("Cannot init mesh with 0 meshes");
        return;
    }

    gf3d_mesh_get_attribute_descriptions(&count);
    mesh_manager.chain_length = gf3d_swapchain_get_chain_length();
    mesh_manager.mesh_list = (Mesh *)gfc_allocate_array(sizeof(Mesh),mesh_max);
    mesh_manager.max_meshes = mesh_max;
    mesh_manager.device = gf3d_vgraphics_get_default_logical_device();

    /* No fucking clue what im supposed to do with this
    faces[0].verts[0] = 2;
    faces[0].verts[1] = 1;
    faces[0].verts[2] = 0;
    faces[1].verts[0] = 1;
    faces[1].verts[1] = 3;
    faces[1].verts[2] = 2;

    bufferSize = sizeof(SpriteFace) * 2;
    */

    mesh_manager.sky_pipe = gf3d_pipeline_create_from_config(
        gf3d_vgraphics_get_default_logical_device(),
        "config/sky_pipeline.cfg",
        gf3d_vgraphics_get_view_extent(),
        mesh_max,
        gf3d_mesh_get_bind_description(),
        gf3d_mesh_get_attribute_descriptions(NULL),
        count,
        sizeof(SkyUBO),
        VK_INDEX_TYPE_UINT16
    );

    mesh_manager.pipe = gf3d_pipeline_create_from_config(
        gf3d_vgraphics_get_default_logical_device(),
        "config/model_pipeline.cfg",
        gf3d_vgraphics_get_view_extent(),
        mesh_max,
        gf3d_mesh_get_bind_description(),
        gf3d_mesh_get_attribute_descriptions(NULL),
        count,
        sizeof(MeshUBO),
        VK_INDEX_TYPE_UINT16
    );

    mesh_manager.defaultTexture = gf3d_texture_load("images/default.png");
    /*if(__DEBUG)slog("mesh manager initiliazed");
    atexit(gf3d_mesh_manager_close);*/

    //probably a good idea to give an exit function
    //atexit()
}

// *
Mesh *gf3d_mesh_get_by_filename(const char *filename)
{
    int i;
    if (!filename)return NULL;
    for (i = 0; i < mesh_manager.max_meshes; i++)
    {

        if (mesh_manager.mesh_list[i]._refCount)continue;
        if (gfc_line_cmp(mesh_manager.mesh_list[i].filename,filename) == 0)
        {
            return &mesh_manager.mesh_list[i];
        }
    }
    //slog("value of %i filename is %s", 1, mesh_manager.mesh_list[i]);
    //slog("Couldnt find mesh by filename of %s", filename);
    return NULL;
}

// *
Mesh *gf3d_mesh_new()
{
    int i;
    for (i = 0; i < mesh_manager.max_meshes; i++)
    {
        if (mesh_manager.mesh_list[i]._refCount)continue;
        mesh_manager.mesh_list[i]._refCount++;
        slog("issuing mesh #%i", i);
        return &mesh_manager.mesh_list[i];
    }
    slog("Mesh limit hit");
    return NULL;
}

Mesh *gf3d_mesh_load(const char *filename)
{
    Mesh *mesh = gf3d_mesh_get_by_filename(filename);
    if (mesh)
    {
        mesh->_refCount++;
        return mesh;
    }
    mesh = gf3d_mesh_new();
    if (!mesh)
    {
        slog("New Mesh returned null");
        return NULL;
    }
    ObjData *obj = gf3d_obj_load_from_file(filename);

    if (!obj)
    {
        slog("Obj data not found for %s", filename);
    }

    gfc_line_cpy(mesh->filename, filename);
    mesh->primitives = gfc_list_new();

    MeshPrimitive *prim = gf3d_mesh_primitive_new();
    prim->objData = obj;
    gf3d_mesh_create_vertex_buffer_from_vertices(prim);
    gf3d_mesh_setup_face_buffers(prim);

    gfc_list_append(mesh->primitives, prim);
    
    return mesh;    
}


void gf3d_mesh_primitive_queue_render(MeshPrimitive *prim,Pipeline *pipe,void *uboData,Texture *texture)
{
    if ((!prim)||(!pipe)||(!uboData))return;
    if (!texture)
    {
        slog("defaulting to default texture");
        texture = mesh_manager.defaultTexture;
    }
    gf3d_pipeline_queue_render(
        pipe,
        prim->vertexBuffer,
        prim->vertexCount,
        prim->faceBuffer,
        uboData,
        texture);
    //slog("drawing a prim");
}

void gf3d_mesh_queue_render(Mesh *mesh,Pipeline *pipe,void *uboData,Texture *texture)
{
    int i,c;
    MeshPrimitive *prim;
    if ((!mesh)||(!pipe)||(!uboData))return;
    c = gfc_list_count(mesh->primitives);
    for (i = 0;i < c; i++)
    {
        prim = gfc_list_nth(mesh->primitives,i);
        if (!prim)continue;
        gf3d_mesh_primitive_queue_render(prim,pipe,uboData,texture);
        
    }
}

void gf3d_sky_draw(Mesh *mesh,GFC_Matrix4 modelMat,GFC_Color mod,Texture *texture)
{
    SkyUBO ubo = {0};
    
    if (!mesh)return;
    gfc_matrix4_copy(ubo.model,modelMat);
    gf3d_vgraphics_get_view(&ubo.view);
    gf3d_vgraphics_get_projection_matrix(&ubo.proj);

    ubo.color = gfc_color_to_vector4f(mod);
    gf3d_mesh_queue_render(mesh,mesh_manager.sky_pipe,&ubo,texture);
}

void gf3d_mesh_draw(Mesh *mesh,GFC_Matrix4 modelMat,GFC_Color mod,Texture *texture,GFC_Vector3D lightPos,GFC_Color lightColor)
{
    MeshUBO ubo = {0};
    
    if (!mesh)
    {
        slog("no mesh to draw");
        return;
    }
    gfc_matrix4_copy(ubo.model,modelMat);
    gf3d_vgraphics_get_view(&ubo.view);
    gf3d_vgraphics_get_projection_matrix(&ubo.proj);

    ubo.color = gfc_color_to_vector4f(mod);
    ubo.lightColor = gfc_color_to_vector4f(lightColor);
    ubo.lightPos = gfc_vector3dw(lightPos,1.0);
    ubo.camera = gfc_vector3dw(gf3d_camera_get_position(),1.0);
    gf3d_mesh_queue_render(mesh,mesh_manager.pipe,&ubo,texture);
}

MeshPrimitive *gf3d_mesh_primitive_new()
{
    MeshPrimitive *prim = (MeshPrimitive *)calloc(1, sizeof(MeshPrimitive));
    return prim;


    //gf3d_mesh_create_vertex_buffer_from_vertices(prim);

}

// *
VkVertexInputBindingDescription * gf3d_mesh_get_bind_description()
{
    mesh_manager.bindingDescription.binding = 0;
    mesh_manager.bindingDescription.stride = sizeof(Vertex);
    mesh_manager.bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return &mesh_manager.bindingDescription;
}

// *
VkVertexInputAttributeDescription * gf3d_mesh_get_attribute_descriptions(Uint32 *count)
{
    mesh_manager.attributeDescriptions[0].binding = 0;
    mesh_manager.attributeDescriptions[0].location = 0;
    mesh_manager.attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    mesh_manager.attributeDescriptions[0].offset = offsetof(Vertex, vertex);
    
    mesh_manager.attributeDescriptions[1].binding = 0;
    mesh_manager.attributeDescriptions[1].location = 1;
    mesh_manager.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    mesh_manager.attributeDescriptions[1].offset = offsetof(Vertex, normal);
    
    mesh_manager.attributeDescriptions[2].binding = 0;
    mesh_manager.attributeDescriptions[2].location = 2;
    mesh_manager.attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    mesh_manager.attributeDescriptions[2].offset = offsetof(Vertex, texel);

    mesh_manager.attributeDescriptions[3].binding = 0;
    mesh_manager.attributeDescriptions[3].location = 3;
    mesh_manager.attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    mesh_manager.attributeDescriptions[3].offset = offsetof(Vertex, bones);

    mesh_manager.attributeDescriptions[4].binding = 0;
    mesh_manager.attributeDescriptions[4].location = 4;
    mesh_manager.attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    mesh_manager.attributeDescriptions[4].offset = offsetof(Vertex, weights);

    if (count)*count = MESH_ATTRIBUTE_COUNT;
    return mesh_manager.attributeDescriptions;
}


void gf3d_mesh_free(Mesh *mesh)
{
    
}


// *
void gf3d_mesh_create_vertex_buffer_from_vertices(MeshPrimitive *primitive)
{
    if (!primitive)
    {
        slog("cant create vertex buffer from null prim");
        return;
    }

    if (!primitive->objData)
    {
        slog("cant create vertex buffer from null objData");
        return;
    }

    void *data = NULL;
    VkDevice device = gf3d_vgraphics_get_default_logical_device();
    Vertex *vertices;
    Uint32 vcount;
    size_t bufferSize;    
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    vertices = primitive->objData->faceVertices;
    vcount = primitive->objData->face_vert_count;
    bufferSize = sizeof(Vertex) * vcount;

    primitive->vertexCount = vcount;

    gf3d_buffer_create(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferMemory
    );

    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    gf3d_buffer_create(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &primitive->vertexBuffer,
        &primitive->vertexBufferMemory
    );

    gf3d_buffer_copy(stagingBuffer, primitive->vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
    
    primitive->vertexCount = vcount;

}

void gf3d_mesh_setup_face_buffers(MeshPrimitive *prim)
{
    void* data = NULL;
    Face *faces = NULL;
    Uint32 fcount;
    
    VkDevice device = gf3d_vgraphics_get_default_logical_device();
    VkDeviceSize bufferSize;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    if ((!prim)||(!prim->objData))return;
    faces = prim->objData->outFace;
    fcount = prim->objData->face_count;
    if ((!faces)||(!fcount))return;
    bufferSize = sizeof(Face) * fcount;
    
    gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, faces, (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &prim->faceBuffer, &prim->faceBufferMemory);

    gf3d_buffer_copy(stagingBuffer, prim->faceBuffer, bufferSize);

    prim->faceCount = fcount;
    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
}

// *
Pipeline *gf3d_mesh_get_pipeline()
{
    return mesh_manager.pipe;
}

/*
MeshUBO gf3d_mesh_get_ubo(GFC_Matrix4 modelMat, GFC_Color colorMod, GFC_Vector3D lightPos, GFC_Color lightColor)
{   
    MeshUBO ubo;
    gfc_matrix4_copy(ubo.model, modelMat);
    gf3d_vgraphics_get_view(ubo.view);
    gf3d_vgraphics_get_projection_matrix(ubo.proj);
    ubo.color = gfc_color_to_vector4(colorMod);
    ubo.camera = gfc_vector3dw(gf3d_camera_get_position(), 1.0);

    ubo.lightpos = gfc_vector3dw(lightPos, 1.0);
    ubo.lightColor = gfc_color_to_vector4(lightColor);
    

    return ubo;
}
    */
