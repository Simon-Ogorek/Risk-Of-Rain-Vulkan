#ifndef __GFC_UI_H__
#define __GFC_UI_H__

#include "gf2d_sprite.h"

typedef struct UI_Element
{
    char name[32];
    GFC_Vector2D elem_pos;
    unsigned int draw_order;
    GFC_TextLine text;
    GFC_Vector2D text_rel_pos; // -1,-1 for centered

}UI_Element;

typedef struct UI_Screen
{
    char name[256];
    UI_Element *elements;
    unsigned int id;
}UI_Screen;

typedef struct UI_Manager
{
    unsigned int active_screen;
    UI_Screen *screens;
}UI_Manager;

void gfc_ui_init(char* config);

int gfc_ui_swap_screen(char* name);

int gfc_ui_edit_text(char* name, GFC_TextLine text);

int gfc_ui_check_clicked(char *name);




#endif