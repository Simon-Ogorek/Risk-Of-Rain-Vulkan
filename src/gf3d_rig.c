#include "simple_logger.h"
#include "gf3d_obj_load.h"
#include "gf3d_mesh.h"
#include "gf3d_buffers.h"
#include "gf3d_swapchain.h"
#include "gf3d_camera.h"
#include "gf3d_vgraphics.h"
#include "gf3d_rig.h"

typedef struct
{
    Rigged_Mesh *rig_list;
    Uint32 rig_max;
} RigManager;

static RigManager rig_manager = {0};

void gf3d_rigged_init(Uint32 max)
{
    if (!max)
    {
        slog("Cannot init rig system with zero rigs");
        return;
    }
    rig_manager.rig_list = (Rigged_Mesh *)gfc_allocate_array(sizeof(Rigged_Mesh), max);
    if (!rig_manager.rig_list)
    {
        slog("failed to allocate entity list");
        return;
    }
    rig_manager.rig_max = max;
}

// *
Rigged_Mesh *gf3d_rigged_get_by_filename(const char *filename)
{
    int i;
    if (!filename)return NULL;
    for (i = 0; i < rig_manager.rig_max; i++)
    {

        if (rig_manager.rig_list[i]._refCount)continue;
        if (gfc_line_cmp(rig_manager.rig_list[i].filename,filename) == 0)
        {
            return &rig_manager.rig_list[i];
        }
    }
    //slog("value of %i filename is %s", 1, mesh_manager.mesh_list[i]);
    //slog("Couldnt find mesh by filename of %s", filename);
    return NULL;
}

// *
Rigged_Mesh *gf3d_rigged_new()
{
    int i;
    for (i = 0; i < rig_manager.rig_max; i++)
    {
        if (rig_manager.rig_list[i]._refCount)continue;
        rig_manager.rig_list[i]._refCount++;
        slog("issuing rig #%i", i);
        return &rig_manager.rig_list[i];
    }
    slog("rig limit hit");
    return NULL;
}

Rigged_Mesh *gf3d_rigged_load(const char *filename)
{
    Rigged_Mesh *rig = gf3d_rigged_get_by_filename(filename);
    if (rig)
    {
        rig->_refCount++;
        return rig;
    }
    rig = gf3d_rigged_new();
    if (!rig)
    {
        slog("New rig returned null");
        return NULL;
    }

    rig->gltf = gf3d_gltf_load(filename);
    if (!rig->gltf)
    {
        slog("GLTF failed to load");
        return NULL;
    }
    SJson *mesh_data = sj_object_get_value(rig->gltf->json, "meshes");
    if (!mesh_data)
    {
        slog("GLTF mesh_data failed");
        return NULL;
    }

    SJson *mesh_single = sj_array_get_nth(mesh_data, 0);
    SJson *prims_data = sj_object_get_value(mesh_single, "primitives");
    if (!prims_data)
    {
        slog("GLTF primitives array failed");
        return NULL;
    }
    // Keep it simple for now, 1 prim per obj
    SJson *prim_data = sj_array_get_nth(prims_data,0);
    if (!prim_data)
    {
        slog("GLTF no prims, failed");
        return NULL;
    }
    rig->mesh = gf3d_mesh_new();
    sj_echo(prim_data);
    ObjData *obj = gf3d_gltf_parse_primitive(rig->gltf, prim_data);
    slog("gltf OBJ: faces: %i, verts: %i, texels: %i", obj->face_count, obj->vertex_count, obj->texel_count);
    /* Test
    Mesh *temp = gf3d_mesh_load("models/enemies/slime.obj");
    MeshPrimitive *temp_prim = (MeshPrimitive *)temp->primitives->elements[0].data;
    ObjData *obj = temp_prim->objData;
    */
    //obj = gf3d_obj_load_from_file("models/GLTF/HAND.obj");
    //slog("reg OBJ: faces: %i, verts: %i, texels: %i", obj->face_count, obj->vertex_count, obj->texel_count);
    if (!obj)
    {
        slog("Obj data not found for %s", filename);
        return NULL;
    }
    slog("got a obj with %i tris from a gltf", obj->face_count);
    
    gfc_line_cpy(rig->filename, filename);
    gfc_line_cpy(rig->mesh->filename, filename);
    rig->mesh->primitives = gfc_list_new();
    MeshPrimitive *prim = gf3d_mesh_primitive_new();
    prim->objData = obj;
    gf3d_mesh_create_vertex_buffer_from_vertices(prim);
    gf3d_mesh_setup_face_buffers(prim);

    gfc_list_append(rig->mesh->primitives, prim);
    
    return rig;    
}