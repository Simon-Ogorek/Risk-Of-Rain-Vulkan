#include "entity.h"
#include "player.h"
#include "world.h"

#define PATH_STALENESS_DIST 20
#define MONST_COUNT 5
typedef struct Path
{
     GFC_List* points;
    int curr_idx;
}Path;

enum Monster_State
{
    UNAWARE, // DEFAULT | Idling
    TRACKING, // Trying to path
    ATTACKING, // Close enough to attack
    RECOIL, // Finished attacking, waiting for end of attack
};
typedef struct Monster_Stats
{
    int health;
    int attack_speed;
    int attack_nums;
    int range;
    int damage;
    int speed;
    int aware_range;
    int time_between_attacks;
    int offsetZ;
    int elemental;
    char *behavior;

}Monster_Stats;

typedef struct Monster
{
    Entity *ent;
    Path* curr_path;
    Monster_Stats* stats;

    enum Monster_State state;
    // This number represents the number of times the enemy has swung in a single attack state
    int attacks; 
    Uint32 attack_time;

    Uint32 recoil_time;
    char* behavior;
}Monster;

void monsters_init(Player* p, World* w);

Monster *spawn_monster();

void monster_navigate_to_player(Monster* tracker);
Path *monster_navigate_to_pos(Monster* tracker, GFC_Vector3D* target);
void monster_kill(Monster* monst);

void monster_think(Entity *ent);

void monster_update(Entity *ent);
void monster_draw(Entity *ent);
void check_player_shot_ray(GFC_Edge3D ray, int damage);
void spawn_random_monster();
void check_player_kill_zone(int damage);

