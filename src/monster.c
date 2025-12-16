#include "simple_logger.h"

#include "monster.h"
#include "entity.h"

#include "gf3d_obj_load.h"
#include "gfc_pak.h"
#include "gfc_config.h"
static World* the_world;
static Player* target_player;
static GFC_List* nav_nodes;
//static GFC_List* 
static GFC_List* active_monsters;
typedef struct NavNode
{
    GFC_Triangle3D tri;
    GFC_Vector3D avg_pos;
    float f;
    float g;
    float h;

    GFC_List *connections;
    int connection_count;

    struct NavNode *parent;
} NavNode;


GFC_Vector3D gfc_average_pos_tri(GFC_Triangle3D tri)
{
    GFC_Vector3D res;

    res = gfc_vector3d_added(tri.a,tri.b);
    res = gfc_vector3d_added(tri.c,res);
    gfc_vector3d_scale(res,res,1.0f/3.0f);
    //slog("computed tri avg of %f,%f,%f", gfc_vector3d_to_slog(res));
    return res;
}

GFC_Vector3D gfc_vector3d_interpolate(GFC_Vector3D a, GFC_Vector3D b, float t)
{
  GFC_Vector3D res, diff;
  gfc_vector3d_sub(diff,b,a);
  gfc_vector3d_scale(diff,diff,t);
  gfc_vector3d_add(res,a,diff);
  return res;
}



void monsters_init(Player* player, World* world)
{
    target_player = player;
    the_world = world;

    nav_nodes = gfc_list_new();
    active_monsters = gfc_list_new();

    MeshPrimitive* prim = (MeshPrimitive *)the_world->walkable->mesh->primitives->elements[0].data;
    ObjData* obj = prim->objData;

    GFC_Matrix4* offset = the_world->walkable->matrix;

    int i,j,k,l;
    GFC_Vector4D out;
    GFC_Triangle3D t;
    NavNode* node;
    NavNode* compare_node;
    if ((!obj)||(!obj->outFace))return 0;
    for (i = 0;i < obj->face_count;i++)
    {
        t.a = obj->faceVertices[obj->outFace[i].verts[0]].vertex;
        t.b = obj->faceVertices[obj->outFace[i].verts[1]].vertex;
        t.c = obj->faceVertices[obj->outFace[i].verts[2]].vertex;
        //apply offset
        gfc_matrix4_multiply_v(&out,offset,gfc_vector3dw(t.a,0));
        t.a = gfc_vector4dxyz(out);
        gfc_matrix4_multiply_v(&out,offset,gfc_vector3dw(t.b,0));
        t.b = gfc_vector4dxyz(out);
        gfc_matrix4_multiply_v(&out,offset,gfc_vector3dw(t.c,0));
        t.c = gfc_vector4dxyz(out);
        
        node = (NavNode*)malloc(sizeof(NavNode));

        node->avg_pos = gfc_average_pos_tri(t);
        node->tri = t;
        node->connection_count = 0;
        node->connections = gfc_list_new();

        gfc_list_append(nav_nodes, node);
    }

    GFC_Triangle3D testTri;
    GFC_Triangle3D Tri;
    for (i = 0;i < gfc_list_get_count(nav_nodes);i++)
    {   
        node = (NavNode*)gfc_list_get_nth(nav_nodes, i);
        Tri = node->tri;
        for (j = 0; j < gfc_list_get_count(nav_nodes);j++)
        {
            if (j == i)
                continue;
            
            k = 0;
            compare_node = (NavNode*)gfc_list_get_nth(nav_nodes,j);
            testTri = compare_node->tri;
            if (gfc_vector3d_compare(Tri.a, testTri.a) ||
                gfc_vector3d_compare(Tri.a, testTri.b) ||
                gfc_vector3d_compare(Tri.a, testTri.c))
            {
                k++;
            }

            if (gfc_vector3d_compare(Tri.b, testTri.a) ||
                gfc_vector3d_compare(Tri.b, testTri.b) ||
                gfc_vector3d_compare(Tri.b, testTri.c))
            {
                k++;
            }

            if (gfc_vector3d_compare(Tri.c, testTri.a) ||
                gfc_vector3d_compare(Tri.c, testTri.b) ||
                gfc_vector3d_compare(Tri.c, testTri.c))
            {
                k++;
            }

            if (k == 2)
            {
                
                gfc_list_append(node->connections, compare_node);
                gfc_list_append(compare_node->connections, node);

                node->connection_count++;
                compare_node->connection_count++;
            }
            
        }
    }
}





NavNode* gfc_find_closest_node(GFC_Vector3D point)
{
    int i;
    NavNode* closest_node = {-999999,-9999999,-333333};
    float closest_dist = 99999999;

    float curr_dist;
    NavNode *node = (NavNode*)gfc_list_get_nth(nav_nodes,0);
    GFC_Vector3D curr;
    for (i = 0;i < gfc_list_get_count(nav_nodes);i++)
    {   
        node = (NavNode*)gfc_list_get_nth(nav_nodes, i);
        curr = node->avg_pos;
        //slog("checking node at pos %f,%f,%f", gfc_vector3d_to_slog(curr));

        curr_dist = gfc_vector3d_magnitude_between(curr,point);

        if (curr_dist < closest_dist)
        {
            closest_dist = curr_dist;
            closest_node = node;
        }   
        //slog("checked vert %f,%f,%f, dist: %f", curr.x, curr.y,curr.z, curr_dist);
    }   
    //slog("Closest tri avg is %f,%f,%f, dist: %f", closest_vert.x, closest_vert.y,closest_vert.z, closest_dist);

    return closest_node;
}

void spawn_random_monster()
{

    char names[MONST_COUNT][20] =
    {
        "friend",
        "bar_knee",
        "sick_ass_spider",
        "goober",
        "count_fresh"
    };

    char *name = names[rand() % MONST_COUNT];

    spawn_monster(name);

}
Monster *spawn_monster(char *name)
{
    Monster* monst = (Monster *)malloc(sizeof(Monster));
    gfc_list_append(active_monsters, monst);
    monst->ent = entity_new(); 

    monst->ent->think = &monster_think;
    monst->ent->update = &monster_update;
    monst->ent->draw = &monster_draw; 
    monst->stats = (Monster_Stats *)malloc(sizeof(Monster_Stats));
    monst->stats->speed = 5;

    monst->ent->calling_parent = (void *)monst;

    gfc_line_cpy(monst->ent->name, "Defaultous Monstorous");

    /*SJson *monst_def = sj_load("config/slime.cfg");
    if (!monst_def)
        slog("fuck");
    char *monst_model = sj_object_get_string(monst_def, "mesh");
    slog("retrieved model path of %s", monst_model);

    char *monst_texture = sj_object_get_string(monst_def, "texture");
    slog("retrieved texture path of %s", monst_texture);
    slog_sync();
    */

    SJson *monst_def_file = sj_load("config/monster.cfg");
    SJson *monst_def = sj_object_get_value(monst_def_file, name);
    sj_echo(monst_def);

    //sj_free(monst_def_file);

    if (!monst_def)
        slog("fuck");
    char *monst_model = sj_object_get_string(monst_def, "mesh");
    slog("retrieved model path of %s", monst_model);

    char *monst_texture = sj_object_get_string(monst_def, "texture");
    slog("retrieved texture path of %s", monst_texture);

    sj_get_integer_value(sj_object_get_value(monst_def,"health"),&monst->stats->health);
    sj_get_integer_value(sj_object_get_value(monst_def,"attack_speed"),&monst->stats->attack_speed);
    sj_get_integer_value(sj_object_get_value(monst_def,"attack_nums"),&monst->stats->attack_nums);
    sj_get_integer_value(sj_object_get_value(monst_def,"range"),&monst->stats->range );
    sj_get_integer_value(sj_object_get_value(monst_def,"damage"),&monst->stats->damage);
    sj_get_integer_value(sj_object_get_value(monst_def,"speed"),&monst->stats->speed);
    sj_get_integer_value(sj_object_get_value(monst_def,"aware_range"),&monst->stats->aware_range);
    sj_get_integer_value(sj_object_get_value(monst_def,"time_between_attacks"),&monst->stats->time_between_attacks);
    sj_get_integer_value(sj_object_get_value(monst_def,"offsetZ"),&monst->stats->offsetZ);
    sj_get_integer_value(sj_object_get_value(monst_def,"elemental"),&monst->stats->elemental);
    sj_get_string_value(sj_object_get_value(monst_def,"behavior"));
    monst->stats->behavior = sj_get_string_value(sj_object_get_value(monst_def,"behavior"));
    slog("awareness : %i | range : %i",monst->stats->aware_range, monst->stats->range);
    
    monst->ent->mesh = gf3d_mesh_load(monst_model);
    monst->ent->texture = gf3d_texture_load(monst_texture);

    monst->ent->lightPos = gfc_vector3d(0,0,5);
    monst->ent->color = GFC_COLOR_WHITE;
    monst->ent->lightColor = GFC_COLOR_WHITE;
    monst->ent->position.z = 5;
    slog("I'm spawning a monster");
    
    GFC_Matrix4 id;
    gfc_matrix4_identity(id);
    gfc_matrix4_rotate_z(monst->ent->matrix,id,0);

    gfc_vector3d_scale(monst->ent->scale, monst->ent->scale,1);
    monst->ent->position.x = -270;
    monst->ent->position.y = 300;
    monst->ent->position.z = 8;
    monst->curr_path = (Path *)malloc(sizeof(Path));
    monst->curr_path->curr_idx = -1;

    monst->curr_path->points = gfc_list_new();

    monst->ent->scale.z = 4;
    monst->ent->scale.x = 4;
    monst->ent->scale.y = 4;


    monst->state = UNAWARE;
    return monst;
}

void attack_player(Monster* monst)
{
    player_damage(tracked_player, monst->stats->damage);
}

Path *monster_navigate_to_pos(Monster* tracker, GFC_Vector3D* target)
{
    return;
}
void monster_navigate_to_player(Monster* tracker)
{
    // NULL
    tracker->curr_path->curr_idx = -1;

    NavNode *start_node;
    NavNode *end_node;
    NavNode *curr_node;
    NavNode *new_node;
    NavNode *temp_node;
    int temp_idx;
    int i,j;
    start_node = gfc_find_closest_node(tracker->ent->position);
    start_node->f = 0;
    start_node->parent = NULL;
    end_node = gfc_find_closest_node(tracked_player->ent->position);

    GFC_List* open = gfc_list_new();
    GFC_List* closed = gfc_list_new();

    //slog("start node is at %f, %f, %f | end node is at %f, %f, %f", gfc_vector3d_to_slog(start_node->avg_pos), gfc_vector3d_to_slog(end_node->avg_pos));
    
    gfc_list_append(open, start_node);

    int nodes_explored = 0;
    int computations = 0;

    while (gfc_list_count(open) > 0)
    {
        curr_node = (NavNode*)gfc_list_get_nth(open,0);
        gfc_list_append(closed, curr_node);
        gfc_list_delete_nth(open,0);
        nodes_explored++;
        //slog("curr node is at %f, %f, %f", gfc_vector3d_to_slog(curr_node->avg_pos));


        if (gfc_vector3d_compare(curr_node->avg_pos, end_node->avg_pos))
        {
            temp_node = curr_node;
            tracker->curr_path->curr_idx = -1;
            gfc_list_delete(tracker->curr_path->points);
            tracker->curr_path->points = gfc_list_new();

            while (temp_node->parent)
            {
                gfc_list_prepend(tracker->curr_path->points,&temp_node->avg_pos);
                tracker->curr_path->curr_idx = 1;
                temp_node = temp_node->parent;
            }
            //slog("Returning a viable path to player of count %i nodes after exploring %i nodes, %i sqrts avoided", gfc_list_count(tracker->curr_path->points), nodes_explored, computations);
            return;
        }
        for (int i = 0; i < curr_node->connection_count; i++)
        {
            new_node = (NavNode*)gfc_list_get_nth(curr_node->connections, i);

            if (gfc_list_get_item_index(closed,new_node) != -1)
                continue;

            new_node->g = curr_node->g + gfc_vector3d_magnitude_between_squared(new_node->avg_pos, curr_node->avg_pos);
            new_node->h = gfc_vector3d_magnitude_between_squared(new_node->avg_pos, tracked_player->ent->position);
            computations += 2;

            new_node->f = new_node->g + new_node->h;

            new_node->parent = curr_node;

            temp_idx = gfc_list_get_item_index(open,new_node);

            if (temp_idx != -1)
            {
                temp_node = gfc_list_get_nth(open,temp_idx);
                // We found a shorter path to this node
                // remove it so it can be correctly replaced
                if (temp_node->f > new_node->f)
                {
                    gfc_list_delete_nth(open, temp_idx);
                }
                else
                {
                    continue;
                }
            }

            temp_idx = gfc_list_get_item_index(closed, new_node);

            if (temp_idx != -1)
            {
                continue;

            }
            int inserted = 0;
            for (j = 0; j < gfc_list_get_count(open); j++)
            {
                if (((NavNode*)gfc_list_get_nth(open,j))->f > new_node->f)
                {
                    gfc_list_insert(open, new_node,j);
                    inserted = 1;
                    break;
                }
            }
            if (!inserted)
            {
                gfc_list_append(open,new_node);
            }


            
        }

    }
    slog("Could not A* a path");
    


    
}
void monster_think(Entity *ent)
{
    Monster* monst = (Monster*)ent->calling_parent;
    //slog("monster think");
    GFC_Vector3D monster_pos = monst->ent->position;
    float aware_range = monst->stats->aware_range;
    float attack_range = monst->stats->range;
    GFC_Vector3D player_pos = tracked_player->ent->position;

    //slog("monster is currently on state %i, %f units away", monst->state, gfc_vector3d_magnitude_between(monster_pos, player_pos));
    if (monst->state == RECOIL)
    {
        if ((SDL_GetTicks() - monst->recoil_time) < monst->stats->time_between_attacks)
        {
            //slog("Still recoiling, %f ms left, returning", (SDL_GetTicks() - monst->recoil_time));
            return;
        }
        else
        {
            monst->state = UNAWARE;
        }
    }
    // if monster is close enough, ignore all previous instructions and start swinging
    if (monst->state != ATTACKING)
    {
        if (gfc_vector3d_magnitude_between(monster_pos, player_pos) < attack_range)
        {
            slog("Player is close enough, switching to attack");
            monst->state = ATTACKING;
            monst->attacks = 0;
            monst->attack_time = 0;
        }
    }

    if (monst->state == ATTACKING)
    {
        if (monst->attacks < monst->stats->attack_nums)
        {
            if ((SDL_GetTicks() - monst->attack_time) > monst->stats->attack_speed)
            {
                slog("damaging player");
                attack_player(monst);
                monst->attack_time = SDL_GetTicks();
                monst->attacks++;
            }
        }
        else
        {
            slog("Entered into recoil");
            monst->state = RECOIL;
            monst->recoil_time = SDL_GetTicks();
        }
    }

    if (monst->state == UNAWARE)
    {
        if (gfc_vector3d_magnitude_between(monster_pos, player_pos) < aware_range)
        {
            monst->state = TRACKING;
            monster_navigate_to_player(monst);
            slog("Was unware, but now tracking the player");
            return;
        }
    }



    if (monst->state != TRACKING)
        return;

    // invalid path, retry
    if (monst->curr_path->curr_idx == -1)
    {
        //slog("monster think");
        monster_navigate_to_player(monst);
        slog("bad path, requesting new one");
        return;
    }
    //slog("currently tracking at path idx of %i", monst->curr_path->curr_idx);
    int ptsCount = gfc_list_get_count(monst->curr_path->points);
    GFC_Vector3D* lastPt = (GFC_Vector3D *)gfc_list_get_nth(monst->curr_path->points, ptsCount - 1);
    float distToPlayer = gfc_vector3d_magnitude_between(monster_pos, player_pos);
    // This path is stale
    if (gfc_vector3d_magnitude_between(*lastPt, player_pos) > PATH_STALENESS_DIST)
    {
        slog("path is stale");
        // Monster should lose interest
        if (distToPlayer < aware_range)
        {
            slog("and player is too far away to care");
            monst->state = UNAWARE;
            return;
        }
        // Monster should keep going and recalculate
        else
        {
            slog("player is close enough to care");
            monster_navigate_to_player(monst);
        }
        
    }
    
    float distLeft = monst->stats->speed;
    // follow the path;
    if (monst->curr_path->curr_idx < gfc_list_get_count(monst->curr_path->points))
    {
        while (distLeft > 0)
        {
            GFC_Vector3D* nextPoint = (GFC_Vector3D *)gfc_list_get_nth(monst->curr_path->points, monst->curr_path->curr_idx);
            float length = gfc_vector3d_magnitude_between(ent->position, *nextPoint);
            GFC_Vector3D dir;
            


            dir = gfc_vector3d_subbed(*nextPoint, monst->ent->position);
            

            dir.z = 0;
            //gfc_vector3d_normalize(&dir);
            
            gfc_vector3d_angles(dir, &monst->ent->rotation);
            
            gfc_vector3d_scale(monst->ent->rotation, monst->ent->rotation, 57.2958);

            if (length < distLeft)
            {
                distLeft -= length;
                ent->position = *nextPoint;
                // smoother movement by ignorning middle verts
                if (monst->curr_path->curr_idx + 4 < gfc_list_count(monst->curr_path->points))
                    monst->curr_path->curr_idx += 3;
                monst->curr_path->curr_idx++;
                //slog("ran 1");
            }
            else
            {
                ent->position = gfc_vector3d_interpolate(ent->position, *nextPoint, distLeft/length);
                distLeft = 0;
                //monst->curr_path->curr_idx++;
                //slog("ran 2");
            }
            
            //slog("enemy pos is %f,%f,%f", monst->ent->position.x,monst->ent->position.y,monst->ent->position.z);

        }
        
    }
    
}
void monster_update(Entity *ent)
{
    GFC_Vector3D preOffset = ent->position;
    ent->position.z += ((Monster*)ent->calling_parent)->stats->offsetZ;
    gfc_matrix4_from_vectors(
        ent->matrix,
        ent->position,
        ent->rotation,
        ent->scale
    );
    ent->position = preOffset;
}
void monster_draw(Entity *ent)
{
    entity_draw(ent);
}

void check_player_shot_ray(GFC_Edge3D ray, int damage)
{
    //slog("checking %i monsters", gfc_list_count(active_monsters));
    for (int i = 0; i < gfc_list_count(active_monsters); i++)
    {
        Monster* monst = (Monster*)gfc_list_get_nth(active_monsters, i);
        MeshPrimitive* monst_prim = (MeshPrimitive *)monst->ent->mesh->primitives->elements[0].data;
        slog("monster is at %f,%f,%f", gfc_vector3d_to_slog(monst->ent->position));
        
        
        if (gf3d_obj_edge_test(monst_prim->objData, monst->ent->matrix, ray, NULL))
        {
            slog("damaged monster from ray");
            monster_damage(monst, damage);
            return;
        }
        
        if (gfc_edge_box_test(ray, monst_prim->objData->bounds, NULL, NULL))
        {
            slog("box test passed");
        }
    }
}

void check_player_kill_zone(int damage)
{
    //slog("checking %i monsters", gfc_list_count(active_monsters));
    for (int i = 0; i < gfc_list_count(active_monsters); i++)
    {
        Monster* monst = (Monster*)gfc_list_get_nth(active_monsters, i);
        
        slog ("monst is %f units away", gfc_vector3d_magnitude_between(monst->ent->position, tracked_player->ent->position));
        if (gfc_vector3d_magnitude_between(monst->ent->position, tracked_player->ent->position) < 500)
        {
            monster_damage(monst, damage);
        }
    }
}

void monster_damage(Monster *monst, int damage)
{
    slog("damage : %i", damage);
    monst->stats->health = ((monst->stats->health - damage) > 0) ? monst->stats->health - damage : 0;
    slog("OWWW!! that hurt, health: %i", monst->stats->health);
    if (!monst->stats->health)
    {
        monster_kill(monst); 
    }
}

void monster_kill(Monster* monst)
{
    slog("killed a monster");
    player_award_kill(tracked_player);

    entity_free(monst->ent);
    
    gfc_list_delete_data(active_monsters, monst);
    free(monst);
}
/*void monster_navigate_to_player(Monster* tracker)
{
    GFC_List* open = gfc_list_new();
    GFC_List* closed = gfc_list_new();
    gfc_list_delete(tracker->curr_path->points);
    tracker->curr_path->points = gfc_list_new();
    GFC_Vector3D groundTestPointEnemy = gfc_vector3d_added(tracker->ent->position, gfc_vector3d(0,0,-100));
    GFC_Edge3D groundTestEdgeEnemy = gfc_edge3d_from_vectors(tracker->ent->position, groundTestPointEnemy);
    GFC_Vector3D contactEnemy;

    GFC_Vector3D groundTestPointPlayer = gfc_vector3d_added(target_player->ent->position, gfc_vector3d(0,0,-100));
    GFC_Edge3D groundTestEdgePlayer = gfc_edge3d_from_vectors(target_player->ent->position, groundTestPointPlayer);
    GFC_Vector3D contactPlayer;

    MeshPrimitive* world_prim = (MeshPrimitive *)the_world->walkable->mesh->primitives->elements[0].data;
    ObjData* world_obj = world_prim->objData;

    GFC_Vector3D closestVertToEnemy;
    GFC_Vector3D* closestVertToPlayer;
    if (gf3d_obj_edge_test(world_prim->objData, the_world->ent->matrix, groundTestEdgeEnemy, &contactEnemy))
    {
        //slog("got a hit for the enemys closest vert %f,%f,%f", contactEnemy.x, contactEnemy.y, contactEnemy.z);

        //contactEnemy.z += 6;
        //gfc_list_clear(tracker->curr_path->points);
        
        gfc_find_closest_vert( world_obj,the_world->ent->matrix,contactEnemy, closestVertToEnemy);
        if (gf3d_obj_edge_test(world_prim->objData, the_world->ent->matrix, groundTestEdgePlayer, &contactPlayer))
        {
            gfc_find_closest_vert( world_obj,the_world->ent->matrix,contactPlayer, *closestVertToPlayer);
            slog("got a hit for the enemys closest vert %f,%f,%f", closestVertToPlayer->x, closestVertToPlayer->y,closestVertToPlayer->z);
        }
        else
        {
            slog("Error: player not close enough to navigatable ground");
        }
        
        //gfc_list_prepend(open,)
    }
    else
    {
        slog("Error: monster not close enough to navigatable ground");
    }
        
    GFC_Vector3D gfc_find_closest_vert(ObjData *obj,GFC_Matrix4 offset, GFC_Vector3D point)
{
    int i;
    GFC_Vector3D closest_vert = {-999999,-9999999,-333333};
    float closest_dist = 99999999;

    float curr_dist;
    GFC_Vector4D temp;
    GFC_Vector3D curr;
    if ((!obj)||(!obj->vertices))return;
    for (i = 0;i < obj->vertex_count;i++)
    {
        curr = obj->vertices[i];
        gfc_matrix4_multiply_v(&temp,offset,gfc_vector3dw(curr,1));
        curr = gfc_vector4dxyz(temp);
        curr_dist = gfc_vector3d_magnitude_between(curr,point);

        if (curr_dist < closest_dist)
        {
            closest_dist = curr_dist;
            closest_vert = curr;
        }   
        //slog("checked vert %f,%f,%f, dist: %f", curr.x, curr.y,curr.z, curr_dist);
    }   
    slog("Closest vert is %f,%f,%f, dist: %f", closest_vert.x, closest_vert.y,closest_vert.z, closest_dist);
}
}*/


