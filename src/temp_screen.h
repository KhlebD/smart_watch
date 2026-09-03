#pragma once
#include <lvgl.h>

void temp_screen_create();
void temp_screen_show();
void temp_screen_update(float temp, float altitude, int hour, int minute);
lv_obj_t* temp_screen_get();