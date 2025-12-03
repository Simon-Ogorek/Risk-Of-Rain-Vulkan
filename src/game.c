#include <SDL.h>            

#include "simple_json.h"
#include "simple_logger.h"

#include "gfc_input.h"
#include "gfc_config_def.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_audio.h"
#include "gfc_string.h"
#include "gfc_actions.h"

#include "gf2d_sprite.h"
#include "gf2d_font.h"
#include "gf2d_actor.h"
#include "gf2d_mouse.h"

#include "gf3d_vgraphics.h"
#include "gf3d_pipeline.h"
#include "gf3d_swapchain.h"
#include "gf3d_camera.h"
#include "gf3d_mesh.h"

#include "entity.h"
#include "agumon.h"
#include "world.h"
#include "player.h"
#include "player_camera.h"
#include "monster.h"

#include "gf3d_gltf_parse.h"

#include "gf3d_rig.h"
extern int __DEBUG;

static int _done = 0;
static Uint32 frame_delay = 33;
static float fps = 0;

// menu_load > main_menu > menu_load > levels (in_game > level_transition) > end_screen
enum game_state {
    menu_load,
    main_menu,
    in_game,
    level_transition,
    end_screen
};

enum game_state curr_state = menu_load;
void parse_arguments(int argc,char *argv[]);
void game_frame_delay();

void exitGame()
{
    _done = 1;
}

/*
void *thread_loading_init(void *arg)
{
    curr_init_step = Initial;

    while (loading)
    {
        switch (curr_init_step)
        {
            case Audio_System:
                
                break;
            
            default:
                break;
        }

    }
    return NULL;

}
*/


int main(int argc,char *argv[])
{
    //local variables
    /*Mesh *mesh;
    Texture *texture;
    float theta = 0;
    
    GFC_Matrix4 id,dinoM;
    GFC_Color lightColor = GFC_COLOR_WHITE;
    GFC_Vector3D lightPos = { 0, 0 ,5};*/
    //initializtion    

    SDL_SetRelativeMouseMode(SDL_TRUE);
    GFC_Vector3D cam = {0,50,500};
    parse_arguments(argc,argv);
    init_logger("gf3d.log",0);
    slog("gf3d begin");
    //gfc init
    gfc_input_init("config/input.cfg");
    gfc_config_def_init();
    gfc_action_init(512);
    //gf3d init
    gf3d_vgraphics_init("config/setup.cfg");
    gf2d_font_init("config/font.cfg");
    gf2d_actor_init(128);

    // Simon's shit
    gf3d_rigged_init(64);
    entity_system_init(64);

    gf2d_sprite_manager_init(128);

    gf3d_texture_init(128);
    
    //game init
    srand(SDL_GetTicks());
    slog_sync();
    
    //gf2d_mouse_load("actors/mouse.actor");
    // main game loop    
    //mesh = gf3d_mesh_load("models/dino/dino.obj");
    //texture = gf3d_texture_load("models/dino/dino.png");
    
    //gfc_matrix4_identity(id);
    
    /*Agumon* agumon_list = (Agumon *)gfc_allocate_array(sizeof(Agumon), 10);

    for (int i = 0; i < 10; i++)
    {
        agumon_list[i] = *spawn_agumon();
    }
    */

    //Agumon* ag1 = spawn_agumon();
    World* world = spawn_world();

    Player* player = spawn_player(world);
    
    P_Camera* p_cam = spawn_camera(player);

    Rigged_Mesh* hand = gf3d_rigged_load("models/GLTF/HANDTri.gltf");

    player_award_kill(player);
    monsters_init(player, world);

    //spawn_random_monster();

    char player_health[10];

    char player_level[10];

    char player_money[10];

    Sprite* heart_pic = gf2d_sprite_load_image("icons/heart.png");
    Sprite* money_pic = gf2d_sprite_load_image("icons/money.png");
    Sprite* xp_pic = gf2d_sprite_load_image("icons/xp.png");
    Sprite* crosshair_pic = gf2d_sprite_load_image("icons/crosshair.png");
    Sprite* shop_pic = gf2d_sprite_load_image("icons/shop.png");
    Uint8 shop_active = 0;
    Uint32 next_enemy_spawn_time = SDL_GetTicks() + (rand()%5000) + 10;
    gfc_audio_init(64,1,0);

    GFC_Sound *boom = gfc_sound_load("audio/vine-boom.mp3", 1.0, 0);

    Texture *hand_text = gf3d_texture_load("models/GLTF/HandTexture.png");


    
    
    //gf3d_camera_look_at(gfc_vector3d(0,0,0),&cam);
    while(!_done)
    {
        

        if (next_enemy_spawn_time < SDL_GetTicks())
        {
            next_enemy_spawn_time = SDL_GetTicks() + (rand()%10) + 10;
            gfc_sound_play(boom, 0, 1.0, -1);
            //spawn_random_monster();
        }
        sprintf(player_health, "%i", player->health);
        sprintf(player_level, "%i", player->level);
        sprintf(player_money, "%i", player->money);
        gfc_input_update();
        gf2d_mouse_update();
        gf2d_font_update();
        //resetMouseToCenter();

        entity_system_think_all();
        entity_system_update_all();

        //world updates
        
        //theta += 0.1;
        //gfc_matrix4_rotate_z(dinoM,id,theta);
        //camera updaes
        gf3d_camera_update_view();
        gf3d_vgraphics_render_start();
        //3D draws
        //gf3d_mesh_draw(mesh,dinoM,GFC_COLOR_WHITE,texture, lightPos, lightColor);
        entity_system_draw_all();
        GFC_Matrix4 mat;
        gfc_matrix4_identity(mat);
        gf3d_mesh_draw(hand->mesh, player->ent->matrix, GFC_COLOR_WHITE, hand_text, gfc_vector3d(0,0,0), GFC_COLOR_WHITE);
        //gf3d_mesh_draw(player->ent->mesh, mat, GFC_COLOR_WHITE, hand_text, gfc_vector3d(0,0,0), GFC_COLOR_WHITE);
        //2D draws

        gf2d_sprite_draw_image(crosshair_pic, gfc_vector2d(1920/2 - 32, 1200/2 - 32));

        gf2d_sprite_draw_image(heart_pic, gfc_vector2d(10,10));
        gf2d_font_draw_line_tag(player_health,FT_H1,GFC_COLOR_WHITE, gfc_vector2d(80,30));

        gf2d_sprite_draw_image(money_pic, gfc_vector2d(1700,10));
        gf2d_font_draw_line_tag(player_money,FT_H1,GFC_COLOR_WHITE, gfc_vector2d(1770,30));
        
        gf2d_sprite_draw_image(xp_pic, gfc_vector2d(150,10));
        gf2d_font_draw_line_tag(player_level,FT_H1,GFC_COLOR_WHITE, gfc_vector2d(220,30));

        if (shop_active)
        {
            gf2d_sprite_draw_image(shop_pic, gfc_vector2d(560,300));
        }
        if (gfc_input_key_pressed("p"))
        {
            shop_active ^= 1;
        }
        gf2d_mouse_draw();
        gf3d_vgraphics_render_end();
        if (gfc_input_command_down("exit"))_done = 1; // exit condition
        game_frame_delay();
    }    
    vkDeviceWaitIdle(gf3d_vgraphics_get_default_logical_device());    
    //cleanup
    slog("gf3d program end");
    exit(0);
    slog_sync();
    return 0;
}

void parse_arguments(int argc,char *argv[])
{
    int a;

    for (a = 1; a < argc;a++)
    {
        if (strcmp(argv[a],"--debug") == 0)
        {
            __DEBUG = 1;
        }
    }    
}

void game_frame_delay()
{
    Uint32 diff;
    static Uint32 now;
    static Uint32 then;
    then = now;
    slog_sync();// make sure logs get written when we have time to write it
    now = SDL_GetTicks();
    diff = (now - then);
    if (diff < frame_delay)
    {
        SDL_Delay(frame_delay - diff);
    }
    fps = 1000.0/MAX(SDL_GetTicks() - then,0.001);
//     slog("fps: %f",fps);
}
/*eol@eof*/