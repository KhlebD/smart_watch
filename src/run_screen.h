#pragma once
#include <lvgl.h>

void run_screen_create();
void run_screen_show(lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_LEFT);
void run_screen_update(float distKm, long timeSec, float paceMinKm,
                       float speedKmh, int hr, const char* state);
lv_obj_t* run_screen_get();
