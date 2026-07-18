#pragma once
#include <lvgl.h>

void hr_screen_create();
void hr_screen_show();
void hr_screen_update(int bpm, long irValue);
lv_obj_t* hr_screen_get();