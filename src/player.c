#include "player.h"
#include "simple_logger.h"
#include "gf3d_mesh.h"
#include "gfc_input.h"
#include "world.h"
#include "physics.h"

#include "gfc_list.h"

#include "gf3d_obj_load.h"

#include "gf2d_mouse.h"
#include "monster.h"

World* curr_world;

Player *spawn_player(World* world)
{
    Player *ag = (Player *)malloc(sizeof(Player));
    
    curr_world = world;
    slog("%p",curr_world);
    ag->ent = entity_new();
    ag->ent->calling_parent = (void *)ag;
    if (ag)
        slog("ag");
    if (ag->ent)
        slog("ent");

    ag->ent->think = &player_think;
    ag->ent->update = &player_update;
    ag->ent->draw = &player_draw; 

    gfc_line_cpy(ag->ent->name, "Player Maximus");

    SJson *player_def = sj_load("config/agumon.cfg");
    if (!player_def)
        slog("fuck");
    char *player_model = sj_object_get_string(player_def, "mesh");
    slog("retrieved model path of %s", player_model);

    char *player_texture = sj_object_get_string(player_def, "texture");
    slog("retrieved texture path of %s", player_texture);
    slog_sync();
    ag->ent->mesh = gf3d_mesh_load(player_model);
    ag->ent->texture = gf3d_texture_load(player_texture);

    ag->ent->lightPos = gfc_vector3d(0,0,5);
    ag->ent->color = GFC_COLOR_WHITE;
    ag->ent->lightColor = GFC_COLOR_WHITE;
    ag->ent->position.z = 5;
    slog("I'm spawning a player");
    
    GFC_Matrix4 id;
    gfc_matrix4_identity(id);
    gfc_matrix4_rotate_z(ag->ent->matrix,id,0);

    gfc_vector3d_scale(ag->ent->scale, ag->ent->scale,1);
    ag->ent->position.z += 50;
    ag->ent->position.x -= 270;


    ag->money = 0;
    ag->dead = 0;
    ag->xp = 0;
    ag->level = 0;
    ag->health = 100;
    ag->original_max_health = 100;
    ag->max_health = 100;
    ag->speed = 2;
    ag->original_damage = 10;
    ag->damage = 10;
    return ag;

    
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

void player_think(Entity *ent)
{
    /*slog("player think");

    */

    //ent->position.z += gfc_random() - 0.5;
    
    
    GFC_Vector3D predicted_move = ent->position;
    Player* play = (Player *)ent->calling_parent;
    float speed = play->speed;
    if (play->dead)
        return;
    if (gfc_input_key_held("a"))
    {
        //slog("LOOK MA, I'm moving left!!!");
        predicted_move.x += speed;
    }
    if (gfc_input_key_held("d"))
    {
        //slog("LOOK MA, I'm moving left!!!");
        predicted_move.x -= speed;
    }
    if (gfc_input_key_held("w"))
    {
        //slog("LOOK MA, I'm moving left!!!");
        predicted_move.y -= speed;
    }
    if (gfc_input_key_held("s"))
    {
        //slog("LOOK MA, I'm moving left!!!");
        predicted_move.y += speed;
    }

    if (gfc_input_key_held(" "))
    {
        //slog("LOOK MA, I'm moving left!!!");
        predicted_move.z += speed;
    }

    if (gfc_input_key_held("c"))
    {
        //slog("LOOK MA, I'm moving left!!!");
        predicted_move.z -= speed;
    }

    
    GFC_Vector3D groundTestPoint = gfc_vector3d_added(predicted_move, gfc_vector3d(0,0,-100));
    GFC_Edge3D groundTestEdge = gfc_edge3d_from_vectors(predicted_move, groundTestPoint);
    GFC_Vector3D contact;

    MeshPrimitive* world_prim = (MeshPrimitive *)curr_world->walkable->mesh->primitives->elements[0].data;

    if (gf3d_obj_edge_test(world_prim->objData, curr_world->ent->matrix, groundTestEdge, &contact))
    {
        //slog("got a hit %f,%f,%f", contact.x, contact.y, contact.z);
        contact.z += 8;
        ent->position = contact ;
    }
    //slog("%f,%f,%f",ent->position.x,ent->position.y,ent->position.z);

    //slog("checking mouse 1 %i", gf2d_mouse_button_pressed(0));
    /*if (gf2d_mouse_button_pressed(0))
    {
        shoot_relative_to_camera();
    }*/

    if (gf2d_mouse_button_pressed(0))
    {
        //slog("M2 being pressed");
        check_player_kill_zone(play->damage);
    }
    

    if (gfc_input_key_pressed("1"))
    {
        if (play->money - 50 >= 0)
        {
            play->money -= 50;
            play->health = (play->health + 40) > play->max_health ? play->max_health : play->health + 40;
        }
        slog("%i",play->max_health);
    }

    if (gfc_input_key_pressed("2"))
    {
        if (play->money - 100 >= 0)
        {
            play->money -= 100;
            play->speed *= 1.2;
        }
    }

    if (gfc_input_key_pressed("3"))
    {
        if (play->money - 100 >= 0)
        {
            play->money -= 100;
            play->max_health += 20;
        }
    }

    if (gfc_input_key_pressed("4"))
    {
        if (play->money - 200 >= 0)
        {
            play->money -= 200;
            play->xp += rand() % 100 + 15;
            play->level = (int)powf(play->xp/10, 0.7);  
            player_stats_scale(play);
        }
    }
    if (gfc_input_key_pressed("5"))
    {
        play->money += 2000;
    }

    return;
}
void player_update(Entity *ent)
{
    //slog("player update");

    gfc_matrix4_from_vectors(
        ent->matrix,
        ent->position,
        ent->rotation,
        ent->scale
    );
}

void player_draw(Entity *ent)
{
    //slog("player draw");
    //entity_draw(ent);
}

void free_player(Player *ag)
{
    if (ag->ent)
        entity_free(ag->ent);
    if (ag)
        free(ag);

    slog("Freeing a player");
}

void player_damage(Player *play, int damage)
{
    play->health = (play->health - damage) > 0 ? play->health - damage : 0;
    
    if (!play->health)
    {
        player_kill(play);
    }
}

void player_kill(Player *play)
{
    play->ent->rotation.y = 90;
    play->dead = 1;
}

void player_award_kill(Player *play)
{
    play->xp += rand() % 15;

    play->level = (int)powf(play->xp/10, 0.7);  
    play->money += rand() % 20;
    player_stats_scale(play);
}

void player_stats_scale(Player* play)
{
    // each level is a 4% gain to stats
    float scale_factor = ((float)play->level + 1) / 25.0f + 1;
    play->attack_speed = scale_factor * play->orginal_attack_speed;
    play->max_health = scale_factor * play->original_max_health;
    play->damage = scale_factor * play->original_damage;
    //slog("new player health");
}