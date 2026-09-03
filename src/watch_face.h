#pragma once
#include <lvgl.h>
 
void watch_face_create(lv_obj_t* screen);
 
// Added bpm, steps, step_goal vs old signature
void watch_face_update(
    int hour, int min, int sec,
    int battery_pct, bool ble_connected,
    int day, int month, int weekday,
    float temp,
    int bpm,
    int steps, int step_goal
);
 
