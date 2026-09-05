#pragma once
#include <lvgl.h>

void show_watch_face(lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_FADE_IN);
void show_step_screen(lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_LEFT);
void show_hr_screen(lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT);
void show_temp_screen(lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_TOP);
void show_music_screen(lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_FADE_IN);