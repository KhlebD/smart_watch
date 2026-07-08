#pragma once
#include <lvgl.h>

void step_screen_create();
void step_screen_update(int steps, int goal);
void step_screen_show();
lv_obj_t* step_screen_get();