#include "step_screen.h"

#include <stdio.h>

#include "globals.h"
#include "main_funcs.h"
#include "watch_face.h"

#define STEP_GOAL 10000

static lv_obj_t* scr_steps = nullptr;
static lv_obj_t* arc_progress = nullptr;
static lv_obj_t* lbl_steps = nullptr;
static lv_obj_t* lbl_goal = nullptr;
static lv_obj_t* lbl_percent = nullptr;
static lv_obj_t* lbl_altitude = nullptr;

static void swipe_cb(lv_event_t* e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        show_watch_face(LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    }
}
lv_obj_t* step_screen_get() { return scr_steps; }

void step_screen_create() {
    scr_steps = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_steps, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr_steps, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_steps, LV_OBJ_FLAG_SCROLLABLE);

    // Swipe down to go back
    lv_obj_add_event_cb(scr_steps, swipe_cb, LV_EVENT_GESTURE, nullptr);

    // Progress arc — background
    arc_progress = lv_arc_create(scr_steps);
    lv_obj_clear_flag(arc_progress, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(arc_progress, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(arc_progress, 200, 200);
    lv_obj_center(arc_progress);
    lv_arc_set_rotation(arc_progress, 270);      // start from 12 o'clock
    lv_arc_set_bg_angles(arc_progress, 0, 360);  // full circle background
    lv_arc_set_angles(arc_progress, 0, 0);       // progress starts at 0
    lv_arc_set_range(arc_progress, 0, 10000);
    lv_arc_set_value(arc_progress, 0);
    lv_obj_remove_style(arc_progress, nullptr, LV_PART_KNOB);  // hide knob

    // Arc background style — dark gray
    lv_obj_set_style_arc_color(arc_progress, lv_color_make(0x22, 0x22, 0x22),
                               LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_progress, 12, LV_PART_MAIN);

    // Arc indicator style — red
    lv_obj_set_style_arc_color(arc_progress, lv_color_make(0xE2, 0x4B, 0x4A),
                               LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_progress, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_progress, true, LV_PART_INDICATOR);

    // Step count label
    lbl_steps = lv_label_create(scr_steps);
    lv_label_set_text(lbl_steps, "0");
    lv_obj_set_style_text_color(lbl_steps, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_steps, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_steps, LV_ALIGN_CENTER, 0, -18);

    // "steps" label
    lv_obj_t* lbl_unit = lv_label_create(scr_steps);
    lv_label_set_text(lbl_unit, "steps");
    lv_obj_set_style_text_color(lbl_unit, lv_color_make(0x88, 0x88, 0x88), 0);
    lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_unit, LV_ALIGN_CENTER, 0, 10);

    // Goal label
    lbl_goal = lv_label_create(scr_steps);
    lv_label_set_text(lbl_goal, "goal: 10,000");
    lv_obj_set_style_text_color(lbl_goal, lv_color_make(0x55, 0x55, 0x55), 0);
    lv_obj_set_style_text_font(lbl_goal, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_goal, LV_ALIGN_CENTER, 0, 38);

    // Percentage label
    lbl_percent = lv_label_create(scr_steps);
    lv_label_set_text(lbl_percent, "0%");
    lv_obj_set_style_text_color(lbl_percent, lv_color_make(0xE2, 0x4B, 0x4A),
                                0);
    lv_obj_set_style_text_font(lbl_percent, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_percent, LV_ALIGN_CENTER, 0, 58);

    // Swipe hint
    lv_obj_t* hint = lv_label_create(scr_steps);
    lv_label_set_text(hint, LV_SYMBOL_DOWN " swipe down");
    lv_obj_set_style_text_color(hint, lv_color_make(0x33, 0x33, 0x33), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);

    // Altitude label
    lbl_altitude = lv_label_create(scr_steps);
    lv_obj_set_style_text_color(lbl_altitude, lv_color_make(0x88, 0x88, 0x88),
                                0);
    lv_obj_set_style_text_font(lbl_altitude, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl_altitude, "-- m");
    lv_obj_align(lbl_altitude, LV_ALIGN_BOTTOM_MID, 0, -35);
}

void step_screen_update(int steps, int goal, float altitude)  {
    if (!scr_steps) return;

    // Format step count with comma
    char stepBuf[12];
    if (steps >= 1000) {
        snprintf(stepBuf, sizeof(stepBuf), "%d,%03d", steps / 1000,
                 steps % 1000);
    } else {
        snprintf(stepBuf, sizeof(stepBuf), "%d", steps);
    }
    lv_label_set_text(lbl_steps, stepBuf);

    // Goal label
    char goalBuf[20];
    snprintf(goalBuf, sizeof(goalBuf), "goal: %d,000", goal / 1000);
    lv_label_set_text(lbl_goal, goalBuf);

    // Altitude
    char altBuf[12];
    snprintf(altBuf, sizeof(altBuf), "%.0f m", altitude);
    lv_label_set_text(lbl_altitude, altBuf);

    // Percentage
    int pct = (int)((float)steps / goal * 100);
    pct = pct > 100 ? 100 : pct;
    char pctBuf[8];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
    lv_label_set_text(lbl_percent, pctBuf);

    // Arc
    lv_arc_set_range(arc_progress, 0, goal);
    lv_arc_set_value(arc_progress, steps > goal ? goal : steps);

    
}

void step_screen_show() {
    lv_scr_load_anim(scr_steps, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}