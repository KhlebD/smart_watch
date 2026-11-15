#pragma once
#include <lvgl.h>
#include "globals.h"
#include "music_screen.h"


void notif_screen_create();
void notif_screen_refresh();
void notif_screen_show(lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_BOTTOM);