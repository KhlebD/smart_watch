#pragma once
#include <lvgl.h>

#define GRAPH_POINTS 30

void hr_screen_create();
void hr_screen_show();
void hr_screen_update(int bpm, long irValue);
void hr_screen_load_history(int* buf, int size, int head, bool full);
void hr_screen_reset_calories();
lv_obj_t* hr_screen_get();