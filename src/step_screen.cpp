#include "step_screen.h"
#include <stdio.h>
#include "globals.h"
#include "main_funcs.h"

#define STEP_GOAL 10000

// ── Widget handles ────────────────────────────────────────────────────────
static lv_obj_t* scr_steps    = nullptr;
static lv_obj_t* lbl_steps    = nullptr;
static lv_obj_t* lbl_progress = nullptr;
static lv_obj_t* lbl_altitude = nullptr;
static lv_obj_t* lbl_distance = nullptr;
static lv_obj_t* lbl_calories = nullptr;
static lv_obj_t* bar_progress = nullptr;

// ── Gesture: swipe right → watch face ────────────────────────────────────
static void swipe_cb(lv_event_t* e) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_RIGHT) show_watch_face(LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    if (dir == LV_DIR_LEFT)  show_hr_screen(LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

lv_obj_t* step_screen_get() { return scr_steps; }

// ── Create ────────────────────────────────────────────────────────────────
void step_screen_create() {
    scr_steps = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_steps, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr_steps, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_steps, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr_steps, swipe_cb, LV_EVENT_GESTURE, nullptr);

    // ── TOP HALF — dark green background ─────────────────────────────────
    lv_obj_t* top = lv_obj_create(scr_steps);
    lv_obj_set_size(top, 240, 120);
    lv_obj_set_pos(top, 0, 0);
    lv_obj_set_style_bg_color(top, lv_color_make(7, 26, 14), 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_radius(top, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    // Steps number — shifted down toward center line
    lbl_steps = lv_label_create(top);
    lv_label_set_text(lbl_steps, "0");
    lv_obj_set_style_text_color(lbl_steps, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_steps, &lv_font_montserrat_28, 0);
    lv_obj_align(lbl_steps, LV_ALIGN_BOTTOM_MID, 0, -22);

    // STEPS unit
    lv_obj_t* lbl_unit = lv_label_create(top);
    lv_label_set_text(lbl_unit, "STEPS");
    lv_obj_set_style_text_color(lbl_unit, lv_color_make(0x39, 0xff, 0x14), 0);
    lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_unit, LV_ALIGN_BOTTOM_MID, 0, -6);

    // ── DIVIDER ───────────────────────────────────────────────────────────
    lv_obj_t* div = lv_obj_create(scr_steps);
    lv_obj_set_size(div, 240, 1);
    lv_obj_set_pos(div, 0, 120);
    lv_obj_set_style_bg_color(div, lv_color_make(0x33, 0x33, 0x33), 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_radius(div, 0, 0);

    // ── BOTTOM HALF ───────────────────────────────────────────────────────
    lv_obj_t* bot = lv_obj_create(scr_steps);
    lv_obj_set_size(bot, 240, 119);
    lv_obj_set_pos(bot, 0, 121);
    lv_obj_set_style_bg_color(bot, lv_color_make(0x0d, 0x0d, 0x0d), 0);
    lv_obj_set_style_bg_opa(bot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bot, 0, 0);
    lv_obj_set_style_radius(bot, 0, 0);
    lv_obj_clear_flag(bot, LV_OBJ_FLAG_SCROLLABLE);

    // Progress bar track
    lv_obj_t* bar_track = lv_obj_create(bot);
    lv_obj_set_size(bar_track, 170, 8);
    lv_obj_align(bar_track, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_bg_color(bar_track, lv_color_make(0x0d, 0x2a, 0x15), 0);
    lv_obj_set_style_bg_opa(bar_track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar_track, 0, 0);
    lv_obj_set_style_radius(bar_track, 4, 0);
    lv_obj_clear_flag(bar_track, LV_OBJ_FLAG_SCROLLABLE);

    // Progress bar fill
    bar_progress = lv_obj_create(bot);
    lv_obj_set_size(bar_progress, 0, 8);
    lv_obj_set_pos(bar_progress, 35, 12);
    lv_obj_set_style_bg_color(bar_progress, lv_color_make(0x39, 0xff, 0x14), 0);
    lv_obj_set_style_bg_opa(bar_progress, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar_progress, 0, 0);
    lv_obj_set_style_radius(bar_progress, 4, 0);

    // Progress text (e.g. "32% of 10,000")
    lbl_progress = lv_label_create(bot);
    lv_label_set_text(lbl_progress, "0% of 10,000");
    lv_obj_set_style_text_color(lbl_progress, lv_color_make(0x39, 0xff, 0x14), 0);
    lv_obj_set_style_text_font(lbl_progress, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_progress, LV_ALIGN_TOP_MID, 0, 26);

    // Altitude
    lbl_altitude = lv_label_create(bot);
    lv_label_set_text(lbl_altitude, " --  m");
    lv_obj_set_style_text_color(lbl_altitude, lv_color_make(0x39, 0xff, 0x14), 0);
    lv_obj_set_style_text_font(lbl_altitude, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_altitude, LV_ALIGN_CENTER, -40, 10);

    // Distance
    lbl_distance = lv_label_create(bot);
    lv_label_set_text(lbl_distance, "--  km");
    lv_obj_set_style_text_color(lbl_distance, lv_color_make(0x39, 0xff, 0x14), 0);
    lv_obj_set_style_text_font(lbl_distance, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_distance, LV_ALIGN_CENTER, 40, 10);

    // Calories
    lbl_calories = lv_label_create(bot);
    lv_label_set_text(lbl_calories, "-- kcal");
    lv_obj_set_style_text_color(lbl_calories, lv_color_make(0x39, 0xff, 0x14), 0);
    lv_obj_set_style_text_font(lbl_calories, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_calories, LV_ALIGN_BOTTOM_MID, 0, -8);
}

// ── Update ────────────────────────────────────────────────────────────────
void step_screen_update(int steps, int goal, float altitude) {
    if (!scr_steps) return;

    // Steps with comma formatting
    char buf[20];
    if (steps >= 1000)
        snprintf(buf, sizeof(buf), "%d,%03d", steps / 1000, steps % 1000);
    else
        snprintf(buf, sizeof(buf), "%d", steps);
    lv_label_set_text(lbl_steps, buf);

    // Progress bar width (max 170px)
    int pct = (int)((float)steps / goal * 100.0f);
    pct = pct > 100 ? 100 : pct;
    int bar_w = (int)(170.0f * steps / goal);
    bar_w = bar_w > 170 ? 170 : bar_w;
    lv_obj_set_width(bar_progress, bar_w);

    // Progress text
    snprintf(buf, sizeof(buf), "%d%% of %d,000", pct, goal / 1000);
    lv_label_set_text(lbl_progress, buf);

    // Altitude
    snprintf(buf, sizeof(buf), "%.0f m", altitude);
    lv_label_set_text(lbl_altitude, buf);

    // Distance (avg 0.75m per step)
    float km = steps * 0.00075f;
    snprintf(buf, sizeof(buf), "%.1f km", km);
    lv_label_set_text(lbl_distance, buf);

    // Calories from steps (simple estimate: steps * weight * 0.00057)
    // ~0.04 kcal per step per 70kg person
    float cal = steps * 70.0f * 0.00057f;
    snprintf(buf, sizeof(buf), "%.0f kcal", cal);
    lv_label_set_text(lbl_calories, buf);
}

// ── Show ──────────────────────────────────────────────────────────────────
void step_screen_show() {
    lv_scr_load_anim(scr_steps, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}