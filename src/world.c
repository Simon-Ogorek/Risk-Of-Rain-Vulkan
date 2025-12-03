#include "world.h"
#include "simple_logger.h"
#include "gf3d_mesh.h"

World *spawn_world()
{
    World *world = (World *)malloc(sizeof(World));
    world->type = WORLD_BASIC;
    world->skybox = entity_new();
    world->ent = entity_new();
    world->walkable = entity_new();

    world->ent->think = &world_think;
    world->ent->update = &world_update;
    world->ent->draw = &world_draw; 

    world->skybox->think = &world_think;
    world->skybox->update = &world_update;
    world->skybox->draw = &world_draw; 

    gfc_line_cpy(world->ent->name, "World McWorldington");
    gfc_line_cpy(world->skybox->name, "Sky McFly");

    SJson *world_def = sj_load("config/world.cfg");
    if (!world_def)
        slog("fuck");
    char *world_model = sj_object_get_string(world_def, "mesh");
    slog("retrieved model path of %s", world_model);

    char *world_texture = sj_object_get_string(world_def, "texture");
    slog("retrieved texture path of %s", world_texture);

    char *world_walkable = sj_object_get_string(world_def, "walkable");
    slog("retrieved walkable path of %s", world_walkable);

    SJson *sky_def = sj_load("config/skybox.cfg");
    if (!sky_def)
        slog("fuck");
    char *sky_model = sj_object_get_string(sky_def, "mesh");
    slog("retrieved model path of %s", sky_model);

    char *sky_texture = sj_object_get_string(sky_def, "texture");
    slog("retrieved texture path of %s", sky_texture);

    slog_sync();
    world->ent->mesh = gf3d_mesh_load(world_model);
    world->ent->texture = gf3d_texture_load(world_texture);

    world->skybox->mesh = gf3d_mesh_load(sky_model);
    world->skybox->texture = gf3d_texture_load(sky_texture);

    world->walkable->mesh = gf3d_mesh_load(world_walkable);

    world->ent->lightPos = gfc_vector3d(0,0,5);
    world->ent->color = GFC_COLOR_WHITE;
    world->ent->lightColor = GFC_COLOR_WHITE;

    slog("I'm spawning a world");
    
    GFC_Matrix4 id;
    gfc_matrix4_identity(id);
    gfc_matrix4_rotate_z(world->ent->matrix,id,0);

    world->ent->color = GFC_COLOR_WHITE;

    //world->ent->rotation.y = 180;
    world->ent->scale = gfc_vector3d(8,8,8);
    world->walkable->scale =  gfc_vector3d(8,8,8);
    gfc_matrix4_identity(id);
    gfc_matrix4_rotate_z(world->walkable->matrix,id,0);

    //GFC_Matrix4 id;
    gfc_matrix4_identity(id);
    gfc_matrix4_rotate_z(world->skybox->matrix,id,0);
    world->skybox->scale = gfc_vector3d(8,8,8);

    gfc_matrix4_from_vectors(
        world->ent->matrix,
        world->ent->position,
        world->ent->rotation,
        world->ent->scale
    );

    gfc_matrix4_from_vectors(
        world->walkable->matrix,
        world->walkable->position,
        world->walkable->rotation,
        world->walkable->scale
    );

    gfc_matrix4_from_vectors(
        world->skybox->matrix,
        world->skybox->position,
        world->skybox->rotation,
        world->skybox->scale
    );
    return world;

    
}

/*
gf3d_mesh_draw(
    ent->mesh, fine
    ent->matrix, ?
    ent->color, ?
    ent->texture, fine
    ent->lightPos, ?
    ent->lightColor ?
);
*/

void world_think(Entity *ent)
{
    /*slog("world think");

    */

    //ent->position.z += gfc_random() - 0.5;
}


void world_update(Entity *ent)
{
    //slog("world update");
}

void world_draw(Entity *ent)
{
    //slog("world draw");
    entity_draw(ent);
}

void free_world(World *world)
{
    if (world->ent)
        entity_free(world->ent);
    if (world)
        free(world);

    slog("Freeing a world");
}
