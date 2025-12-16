#include "simple_logger.h"
#include "gf3d_obj_load.h"
#include "gf3d_mesh.h"
#include "gf3d_buffers.h"
#include "gf3d_swapchain.h"
#include "gf3d_camera.h"
#include "gf3d_vgraphics.h"
#include "gf3d_rig.h"
#include "gfc_decode.h"

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
        gf3d_rigged_parse_armarture(rig);
        return rig;    
}

void gf3d_rigged_advance_animation(Rigged_Mesh* mesh)
{
    mesh->arm->animation->current_frame++;
    mesh->arm->animation->current_frame%=(mesh->arm->animation->frame_count-1);
    GLTF_Node *temp;
    
    gfc_matrix4_copy(mesh->arm->animation->inverse_bind, mesh->arm->animation->inverse_bind_data);

    Channel* channel;
    Sampler* sample;
    GLTF_Node* target;
    GFC_Vector3D *VEC3_data;
    GFC_Vector4D *VEC4_data;

    for (int i = 0; i < gfc_list_get_count(mesh->arm->skin); i++)
    {
        int jointMapping = *(int *)gfc_list_get_nth(mesh->arm->skin, i);

        target = NULL;
        for (int j = 0 ; j < gfc_list_get_count(mesh->arm->bones); j++)
        {
            GLTF_Node * test = gfc_list_get_nth(mesh->arm->bones,j);
            if (test->index == jointMapping)
            {
                target = test;
                break;
            }
        }
        if (!target)
        {
            slog("NULL BONE");
            return;
        }
        gfc_matrix4_copy(target->inverse_bind, mesh->arm->animation->inverse_bind_data[i]);
        gfc_matrix4_to_vectors(target->base_bone_matrix,&target->translation, &target->rotation, &target->scale);
    }

    for (int i = 0; i < gfc_list_get_count(mesh->arm->animation->channels); i++)
    {
        channel = gfc_list_get_nth(mesh->arm->animation->channels, i);
        target =  channel->target;
        sample = channel->sample;

        if (sample->type == VEC3)
        {
            VEC3_data = ((GFC_Vector3D*)sample->data) + mesh->arm->animation->current_frame;
            if (channel->type == Translation)
            {
                target->translation.x = VEC3_data->x;
                target->translation.y = VEC3_data->y;
                target->translation.z = VEC3_data->z;
            }
            else if (channel->type == Scale)
            {
                target->scale.x = VEC3_data->x;
                target->scale.y = VEC3_data->y;
                target->scale.z = VEC3_data->z;
            }
            else
            {
                slog("fucked up channel type");
            }
        }
        else if (sample->type == VEC4)
        {
            VEC4_data = ((GFC_Vector4D*)sample->data) + mesh->arm->animation->current_frame;
            if (channel->type == Rotation)
            {
                target->rotation.x = VEC4_data->x;
                target->rotation.y = VEC4_data->y;
                target->rotation.z = VEC4_data->z;
                target->rotation.w = VEC4_data->w;
            }
            else
            {
                slog("fucked up channel type");
            }
        }
        else
        {
            slog("Undefined sample type");
        }
    }

    for (int i = 0; i < gfc_list_get_count(mesh->arm->bones); i++)
    {
        temp = (GLTF_Node*)gfc_list_get_nth(mesh->arm->bones, i);
        update_matrices(temp);
    }


}

void gf3d_rigged_draw(Rigged_Mesh *mesh,GFC_Matrix4 modelMat,GFC_Color mod,Texture *texture,GFC_Vector3D lightPos,GFC_Color lightColor)
{

    //gf3d_rigged_advance_animation(mesh);
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
    GLTF_Node* bone;
    //slog("There are %i bones", gfc_list_get_count(mesh->arm->bones));

    for (int i = 0; i < 64; i++)
    {
        gfc_matrix4_identity(ubo.bones[i]);
    }
    for (int i = 0; i < gfc_list_get_count(mesh->arm->skin); i++)
    {
        int jointMapping = *(int*)gfc_list_get_nth(mesh->arm->skin, i);
        bone = NULL;
        for (int j = 0; j < gfc_list_get_count(mesh->arm->skin); j++)
        {
            GLTF_Node *test = (GLTF_Node *)gfc_list_get_nth(mesh->arm->bones, j);
            if (test->index == jointMapping)
            {
                bone = test;
                break;          
            }
        }
        if (!bone)
        {
            slog("NULL BONE");
            return;
        }
        
        //slog("ubo is getting %s with a pos of %f,%f,%f", gfc_vector3d_to_slog(bone->translation), bone->name);
        gfc_matrix4_multiply(ubo.bones[i], bone->global_matrix, bone->inverse_bind);
    }
    gf3d_mesh_queue_render(mesh,mesh_manager.pipe,&ubo,texture);
}

void update_children_matrices(GLTF_Node *parent)
{
    GLTF_Node * temp;
    for (int i = 0; i < gfc_list_count(parent->children); i++)
    {
        temp = (GLTF_Node*) gfc_list_get_nth(parent->children, i);

        gfc_matrix4_from_vectors_q(temp->local_matrix, temp->translation, temp->rotation, temp->scale);
        gfc_matrix4_multiply(temp->global_matrix, parent->global_matrix, temp->local_matrix);

        if (gfc_list_count(temp->children))
        {
            update_children_matrices(temp);
        }
    }
}

void update_matrices(GLTF_Node* node)
{
    if (node->root)
    {
        gfc_matrix4_from_vectors_q(node->local_matrix, node->translation, node->rotation, node->scale);
        gfc_matrix4_copy(node->global_matrix, node->local_matrix);
        update_children_matrices(node);
    }
}

GLTF_Node* parse_node(Rigged_Mesh* mesh, SJson* node)
{
    SJson* rotationInfo = sj_object_get_value(node, "rotation");
    SJson* scaleInfo = sj_object_get_value(node, "scale");
    SJson* translationInfo = sj_object_get_value(node, "translation");

    // This curr node is about the mesh or armature not bones
    if (!rotationInfo && !scaleInfo && !translationInfo)
        return NULL;

    GLTF_Node *temp = (GLTF_Node *) malloc(sizeof(GLTF_Node));
    temp->scale.x = 1;
    temp->scale.y = 1;
    temp->scale.z = 1;

    if (rotationInfo)
    {
        sj_get_float_value(sj_array_get_nth(rotationInfo,0), &temp->rotation.x);
        sj_get_float_value(sj_array_get_nth(rotationInfo,1), &temp->rotation.y);
        sj_get_float_value(sj_array_get_nth(rotationInfo,2), &temp->rotation.z);
        sj_get_float_value(sj_array_get_nth(rotationInfo,3), &temp->rotation.w);
    }

    if (scaleInfo)
    {
        sj_get_float_value(sj_array_get_nth(scaleInfo,0), &temp->scale.x);
        sj_get_float_value(sj_array_get_nth(scaleInfo,0), &temp->scale.y);
        sj_get_float_value(sj_array_get_nth(scaleInfo,0), &temp->scale.z);
    }

    if (translationInfo)
    {
        sj_get_float_value(sj_array_get_nth(translationInfo,0), &temp->translation.x);
        sj_get_float_value(sj_array_get_nth(translationInfo,0), &temp->translation.y);
        sj_get_float_value(sj_array_get_nth(translationInfo,0), &temp->translation.z);
    }

    temp->name = sj_get_string_value(sj_object_get_value(node, "name"));
    SJson* childrenInfo = sj_object_get_value(node, "children");
    if (childrenInfo)
    {
        temp->children = gfc_list_new();
        
        for (int i = 0; i < sj_array_get_count(childrenInfo); i++)
        {
            int childIdx;
            sj_get_integer_value(sj_array_get_nth(childrenInfo, i), &childIdx);
            gfc_list_append(temp->children, &childIdx);
        }
    }
    else
    {
        temp->children = NULL;
        temp->root = 0;
    }

    return temp;
    
}
void gf3d_rigged_parse_armarture(Rigged_Mesh* mesh)
{
    slog("parsing animation");
    // figure out how many nodes we are dealing with
    SJson *scene = sj_object_get_value(mesh->gltf->json, "scenes");
    SJson *sceneInfo = sj_array_get_nth(scene, 0);
    SJson *nodeInfo = sj_object_get_value(sceneInfo, "nodes");
    SJson *nodeCountInfo = sj_array_get_nth(nodeInfo, 0);

    SJson *accessors_info = sj_object_get_value(mesh->gltf->json, "accessors");
    SJson *animations_info = sj_object_get_value(mesh->gltf->json,"animations");
    SJson *buffer_views = sj_object_get_value(mesh->gltf->json, "bufferViews");

    SJson *animation_info = sj_array_get_nth(animations_info, 0);
    SJson *action_info = sj_object_get_value(animation_info, "samplers");
    SJson *channels_info = sj_object_get_value(animation_info, "channels");

    SJson *skinsInfo = sj_object_get_value(mesh->gltf->json, "skins");
    SJson *skinInfo = sj_array_get_nth(skinsInfo, 0);

    SJson *buffer = sj_array_get_nth(sj_object_get_value(mesh->gltf->json, "buffers"),0);

    mesh->arm = (Armature *)malloc(sizeof(Armature));
    mesh->arm->animation = (Animation *)malloc(sizeof(Animation));
    slog("Animation is defined at %p", mesh->arm->animation);
    mesh->arm->animation->current_frame = 0;
    // get the actual buffer data
    
    const char *data = sj_object_get_value_as_string(buffer,"uri");
    if (!data)return;
        
    data = strchr(data, ',');
    data++;
    data = gfc_base64_decode (data, strlen(data), NULL);


    int matrix_accessor_idx;
    sj_get_integer_value(sj_object_get_value(skinInfo,"inverseBindMatrices"), &matrix_accessor_idx);
    SJson* matrix_accessor = sj_array_get_nth(accessors_info, matrix_accessor_idx);
    int matrix_bufferview_idx;
    int matrix_count;
    sj_get_integer_value(sj_object_get_value(matrix_accessor,"bufferView"), &matrix_bufferview_idx);
    sj_get_integer_value(sj_object_get_value(matrix_accessor,"count"), &matrix_count);

    int byteLength, byteOffset;
    SJson *buffer_info = sj_array_get_nth(buffer_views, matrix_bufferview_idx);
    sj_get_integer_value(sj_object_get_value(buffer_info,"byteLength"), &byteLength);
    sj_get_integer_value(sj_object_get_value(buffer_info,"byteOffset"), &byteOffset);

    mesh->arm->animation->inverse_bind_data = (GFC_Matrix4 *)malloc(sizeof(GFC_Matrix4) * matrix_count);
    memcpy(mesh->arm->animation->inverse_bind_data,&data[byteOffset],byteLength);

    mesh->arm->skin = gfc_list_new();
    SJson *jointInfo = sj_object_get_value(skinInfo, "joints");
    //int joint_idxs[sj_array_get_count(jointInfo)];
    for (int i = 0; i < sj_array_get_count(jointInfo); i++)
    {
        int *joint_idx = malloc(sizeof(int));
        sj_get_integer_value(sj_array_get_nth(jointInfo, i),joint_idx);
        gfc_list_append(mesh->arm->skin,joint_idx);
    }

    int nodeCount;
    if (!sj_get_integer_value(nodeCountInfo, &nodeCount))
    {
        slog("Failed to parse scene info");
        return;
    }
    

    // Figure out if a node is a bone and add it to the armature

    mesh->arm->bones = gfc_list_new();
    SJson *nodes = sj_object_get_value(mesh->gltf->json, "nodes");

    for (int i = 0; i < nodeCount; i++)
    {
        GLTF_Node* parsed = parse_node(mesh, sj_array_get_nth(nodes,i));
        if (!parsed)
            continue;
        parsed->index = i;
        gfc_list_append(mesh->arm->bones, parsed);
    }

    // if the very last node isnt a bone, assume its a armature def
    if (!parse_node(mesh, sj_array_get_nth(nodes,nodeCount-1)))
    {
        SJson* childrenInfo = sj_object_get_value(sj_array_get_nth(nodes,nodeCount-1), "children");

        // go back and make the children of the armature root bones
        for (int i = 0; i < sj_array_get_count(childrenInfo); i++)
        {
            int rootIdx;
            sj_get_integer_value(sj_array_get_nth(childrenInfo, i), &rootIdx);
            for (int j = 0; j > gfc_list_get_count(mesh->arm->bones); j++)
            {
                GLTF_Node *temp = (GLTF_Node*)gfc_list_get_nth(mesh->arm->bones, j);
                if (temp->index == rootIdx)
                {
                    temp->root = 1;
                    slog("Made bone %s a root bone", temp->name);
                }
            }
        }
    }



    Uint8 inputRead = 0;


    mesh->arm->animation->channels = gfc_list_new();

    // Parse each channel
    for (int j = 0; j < sj_array_get_count(channels_info); j++ )
    {
        SJson *channel_info = sj_array_get_nth(channels_info, j);
        SJson *target_info = sj_object_get_value(channel_info, "target");

        Channel *temp_channel = (Channel *)malloc(sizeof(Channel));
        int target_idx;
        sj_get_integer_value(sj_object_get_value(target_info,"node"),&target_idx);
        temp_channel->target = (GLTF_Node *)gfc_list_get_nth(mesh->arm->bones, target_idx);
        const char* path = sj_get_string_value(sj_object_get_value(target_info, "path"));

        if (!strcmp(path, "rotation"))
        {
            temp_channel->type = Rotation;
        }
        else if (!strcmp(path, "translation"))
        {
            temp_channel->type = Translation;
        }
        else if (!strcmp(path, "scale"))
        {
            temp_channel->type = Scale;
        }
        else
        {
            slog("No type found for a channel");
        }

        temp_channel->sample = (Sampler *)malloc(sizeof(Sampler));
        int sample_idx = 0;
        sj_get_integer_value(sj_object_get_value(channel_info, "sampler"), &sample_idx);
        SJson* sample_info = sj_array_get_nth(action_info, sample_idx);

        // the input is the same for all at this basic level, dont burn time reading it multiple times
        if (!inputRead)
        {
            int input_idx = 0;
            sj_get_integer_value(sj_object_get_value(sample_info, "input"),&input_idx);
            SJson* input_info = sj_array_get_nth(accessors_info, input_idx);
            sj_get_integer_value(input_info,&mesh->arm->animation->frame_count);
            //mesh->arm->animation->frame_end = mesh->arm->animation->frame_count - 1;
            sj_get_float_value(sj_array_get_nth(sj_object_get_value(input_info, "max"),0),&mesh->arm->animation->seconds_end);
            sj_get_float_value(sj_array_get_nth(sj_object_get_value(input_info, "min"),0),&mesh->arm->animation->seconds_start);
            mesh->arm->animation->seconds_duration = mesh->arm->animation->seconds_end - mesh->arm->animation->seconds_start;
            inputRead = 1;
        }

        // figure out which buffer to find
        int output_idx = 0;
        sj_get_integer_value(sj_object_get_value(sample_info, "output"),&output_idx);
        SJson* output_info = sj_array_get_nth(accessors_info, output_idx);

        char *buffer_type = sj_get_string_value(sj_object_get_value(output_info, "type"));

        // do some buffer bs, define type, length, and struct type

        if (!strcmp(buffer_type, "VEC4"))
        {
            temp_channel->sample->type = VEC4;
        }
        else if (!strcmp(buffer_type, "VEC3"))
        {
            temp_channel->sample->type = VEC3;
        }
        else
        {
            slog("No type found for a sample");
        }

        int bufferView_idx = 0;
        sj_get_integer_value(sj_object_get_value(output_info, "bufferView"),&bufferView_idx);
        sj_get_integer_value(sj_object_get_value(output_info, "count"), &temp_channel->sample->count);

        SJson *buffer_info = sj_array_get_nth(buffer_views, bufferView_idx);
        sj_get_integer_value(sj_object_get_value(buffer_info,"byteLength"), &byteLength);
        sj_get_integer_value(sj_object_get_value(buffer_info,"byteOffset"), &byteOffset);

        temp_channel->sample->data = (void *)malloc(byteLength);
        memcpy(temp_channel->sample->data,&data[byteOffset],byteLength);

        gfc_list_append(mesh->arm->animation->channels, temp_channel);
    }

    for (int i = 0; i < gfc_list_count(mesh->arm->bones); i++)
    {
        GLTF_Node *curr_node = (GLTF_Node*)gfc_list_get_nth(mesh->arm->bones,i);

        if (curr_node)
            gfc_matrix4_from_vectors_q(curr_node->base_bone_matrix, curr_node->translation, curr_node->rotation, gfc_vector3d(1,1,1));

    }
}