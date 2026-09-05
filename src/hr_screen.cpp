#include <stdio.h>
#include "hr_screen.h"
#include "globals.h"
#include "main_funcs.h"

#define NO_FINGER_THRESHOLD 50000

// ── User config ───────────────────────────────────────────────────────────
#define USER_WEIGHT_KG 100.0f
#define USER_AGE 30

// ── Widget handles ────────────────────────────────────────────────────────
static lv_obj_t* scr_hr = nullptr;
static lv_obj_t* lbl_bpm = nullptr;
static lv_obj_t* lbl_status = nullptr;
static lv_obj_t* lbl_calories = nullptr;
static lv_obj_t* chart = nullptr;
static lv_chart_series_t* ser = nullptr;

// ── Calorie tracking ──────────────────────────────────────────────────────
static float total_calories = 0.0f;
static uint32_t last_cal_update = 0;

// ── Gesture: swipe left → watch face ─────────────────────────────────────
static void swipe_cb(lv_event_t *e) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_RIGHT) show_step_screen(LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    if (dir == LV_DIR_LEFT)  show_watch_face(LV_SCR_LOAD_ANIM_MOVE_LEFT);
}
// ── Create ────────────────────────────────────────────────────────────────
void hr_screen_create() {
    scr_hr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_hr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr_hr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_hr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr_hr, swipe_cb, LV_EVENT_GESTURE, nullptr);

    // ── TOP HALF — dark red background ───────────────────────────────────
    lv_obj_t* top = lv_obj_create(scr_hr);
    lv_obj_set_size(top, 240, 120);
    lv_obj_set_pos(top, 0, 0);
    lv_obj_set_style_bg_color(top, lv_color_make(26, 8, 8), 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_radius(top, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    // BPM number (large, top half)
    lbl_bpm = lv_label_create(top);
    lv_label_set_text(lbl_bpm, "--");
    lv_obj_set_style_text_color(lbl_bpm, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_bpm, &lv_font_montserrat_28, 0);
    lv_obj_align(lbl_bpm, LV_ALIGN_CENTER, 0, -12);

    // BPM unit label
    lv_obj_t* lbl_unit = lv_label_create(top);
    lv_label_set_text(lbl_unit, "BPM");
    lv_obj_set_style_text_color(lbl_unit, lv_color_make(0xff, 0x40, 0x10), 0);
    lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_unit, LV_ALIGN_CENTER, 0, 14);

    // Calories label (top right corner)
    lbl_calories = lv_label_create(top);
    lv_label_set_text(lbl_calories, "0 kcal");
    lv_obj_set_style_text_color(lbl_calories, lv_color_make(0xaa, 0x55, 0x55),
                                0);
    lv_obj_set_style_text_font(lbl_calories, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_calories, LV_ALIGN_CENTER, 0, 38);

    // ── DIVIDER ───────────────────────────────────────────────────────────
    lv_obj_t* div = lv_obj_create(scr_hr);
    lv_obj_set_size(div, 240, 1);
    lv_obj_set_pos(div, 0, 120);
    lv_obj_set_style_bg_color(div, lv_color_make(0x33, 0x33, 0x33), 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_radius(div, 0, 0);

    // ── BOTTOM HALF — graph ───────────────────────────────────────────────
    lv_obj_t* bot = lv_obj_create(scr_hr);
    lv_obj_set_size(bot, 240, 119);
    lv_obj_set_pos(bot, 0, 121);
    lv_obj_set_style_bg_color(bot, lv_color_make(0x0d, 0x0d, 0x0d), 0);
    lv_obj_set_style_bg_opa(bot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bot, 0, 0);
    lv_obj_set_style_radius(bot, 0, 0);
    lv_obj_clear_flag(bot, LV_OBJ_FLAG_SCROLLABLE);

    // Chart
    chart = lv_chart_create(bot);
    lv_obj_set_size(chart, 220, 100);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, -8);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, GRAPH_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 40, 180);
    lv_chart_set_div_line_count(chart, 0, 0);

    lv_obj_set_style_bg_color(chart, lv_color_make(0x0d, 0x0d, 0x0d), 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_radius(chart, 0, 0);
    lv_obj_set_style_pad_all(chart, 0, 0);

    ser = lv_chart_add_series(chart, lv_color_make(0xff, 0x40, 0x10),
                              LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);

    for (int i = 0; i < GRAPH_POINTS; i++)
        lv_chart_set_next_value(chart, ser, 0);

    // Status label
    lbl_status = lv_label_create(bot);
    lv_label_set_text(lbl_status, "place finger on sensor");
    lv_obj_set_style_text_color(lbl_status, lv_color_make(0x44, 0x44, 0x44), 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_status, LV_ALIGN_BOTTOM_MID, 0, -6);
}

// ── Load history into chart ───────────────────────────────────────────────
void hr_screen_load_history(int* buf, int size, int head, bool full) {
    if (!chart || !ser) return;

    for (int i = 0; i < GRAPH_POINTS; i++)
        lv_chart_set_next_value(chart, ser, 0);

    int count = full ? size : head;
    int start = full ? head : 0;
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % size;
        if (buf[idx] > 0) lv_chart_set_next_value(chart, ser, buf[idx]);
    }
    lv_chart_refresh(chart);
}

// ── Update ────────────────────────────────────────────────────────────────
void hr_screen_update(int bpm, long irValue) {
    if (!scr_hr) return;

    bool fingerOn = irValue > NO_FINGER_THRESHOLD;

    // ── Calorie update (every minute) ─────────────────────────────────────
    uint32_t now = lv_tick_get();
    if (now - last_cal_update >= 60000) {
        last_cal_update = now;
        if (fingerOn && bpm > 0) {
            float cal_per_min =
                (-55.0969f + 0.6309f * bpm + 0.1988f * USER_WEIGHT_KG +
                 0.2017f * USER_AGE) /
                4.184f;
            if (cal_per_min > 0) total_calories += cal_per_min;
        }
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f kcal", total_calories);
        lv_label_set_text(lbl_calories, buf);
    }

    // ── BPM display ───────────────────────────────────────────────────────
    if (fingerOn && bpm > 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", bpm);
        lv_label_set_text(lbl_bpm, buf);
        lv_obj_set_style_text_color(lbl_bpm, lv_color_white(), 0);
        lv_chart_set_next_value(chart, ser, bpm);
        lv_chart_refresh(chart);

    } else if (fingerOn) {
        lv_label_set_text(lbl_bpm, "--");
        lv_obj_set_style_text_color(lbl_bpm, lv_color_white(), 0);


    } else {
        lv_label_set_text(lbl_bpm, "--");
        lv_obj_set_style_text_color(lbl_bpm, lv_color_make(0x44, 0x44, 0x44),
                                    0);
        lv_label_set_text(lbl_status, "Not Measuring");
        lv_obj_set_style_text_color(lbl_status, lv_color_make(0x44, 0x44, 0x44),
                                    0);
    }
}

// ── Reset calories (call at midnight) ────────────────────────────────────
void hr_screen_reset_calories() {
    total_calories = 0.0f;
    last_cal_update = 0;
    if (lbl_calories) lv_label_set_text(lbl_calories, "0 kcal");
}

// ── Show ──────────────────────────────────────────────────────────────────
void hr_screen_show() {
    lv_scr_load_anim(scr_hr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

lv_obj_t* hr_screen_get() { return scr_hr; }