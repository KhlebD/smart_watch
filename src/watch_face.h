#pragma once
#include <lvgl.h>

void watch_face_create(lv_obj_t* scr);
void watch_face_update(int hours, int minutes, int seconds, int battery,
                       bool ble_connected, int day, int month,
                       float temperature);

