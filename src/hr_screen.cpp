#include "hr_screen.h"
#include "globals.h"
#include "main_funcs.h"
#include <stdio.h>

#define GRAPH_POINTS    30
#define NO_FINGER_THRESHOLD 50000

static lv_obj_t *scr_hr       = nullptr;
static lv_obj_t *lbl_bpm      = nullptr;
static lv_obj_t *lbl_status   = nullptr;
static lv_obj_t *chart        = nullptr;
static lv_chart_series_t *ser = nullptr;

static void swipe_cb(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_LEFT) {
        show_watch_face(LV_SCR_LOAD_ANIM_MOVE_LEFT);
    }
}

void hr_screen_create() {
    scr_hr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_hr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr_hr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_hr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr_hr, swipe_cb, LV_EVENT_GESTURE, nullptr);

    // Header
    lv_obj_t *hdr = lv_label_create(scr_hr);
    lv_label_set_text(hdr, "heart rate");
    lv_obj_set_style_text_color(hdr, lv_color_make(0x88, 0x88, 0x88), 0);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 10);

    // BPM number
    lbl_bpm = lv_label_create(scr_hr);
    lv_label_set_text(lbl_bpm, "--");
    lv_obj_set_style_text_color(lbl_bpm, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_bpm, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_bpm, LV_ALIGN_TOP_MID, 0, 32);

    // BPM label
    lv_obj_t *lbl_unit = lv_label_create(scr_hr);
    lv_label_set_text(lbl_unit, "BPM");
    lv_obj_set_style_text_color(lbl_unit, lv_color_make(0xE2, 0x4B, 0x4A), 0);
    lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_unit, LV_ALIGN_TOP_MID, 0, 62);

    // Graph
    chart = lv_chart_create(scr_hr);
    lv_obj_set_size(chart, 200, 90);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 20);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, GRAPH_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 40, 180);
    lv_chart_set_div_line_count(chart, 3, 0);

    // Chart style
    lv_obj_set_style_bg_color(chart, lv_color_make(0x1A, 0x1A, 0x1A), 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_radius(chart, 8, 0);
    lv_obj_set_style_line_color(chart, lv_color_make(0x2A, 0x2A, 0x2A), LV_PART_MAIN);

    // Series
    ser = lv_chart_add_series(chart, lv_color_make(0xE2, 0x4B, 0x4A), LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR); // hide dots

    // Fill series with 0 initially
    for (int i = 0; i < GRAPH_POINTS; i++) {
        lv_chart_set_next_value(chart, ser, 0);
    }

    // Status label
    lbl_status = lv_label_create(scr_hr);
    lv_label_set_text(lbl_status, "place finger on sensor");
    lv_obj_set_style_text_color(lbl_status, lv_color_make(0x55, 0x55, 0x55), 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_status, LV_ALIGN_BOTTOM_MID, 0, -25);

    // Swipe hint
    lv_obj_t *hint = lv_label_create(scr_hr);
    lv_label_set_text(hint, LV_SYMBOL_DOWN " swipe down");
    lv_obj_set_style_text_color(hint, lv_color_make(0x33, 0x33, 0x33), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
}

void hr_screen_update(int bpm, long irValue) {
    if (!scr_hr) return;

    bool fingerOn = irValue > NO_FINGER_THRESHOLD;

    if (fingerOn && bpm > 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", bpm);
        lv_label_set_text(lbl_bpm, buf);
        lv_obj_set_style_text_color(lbl_bpm, lv_color_white(), 0);
        lv_label_set_text(lbl_status, "measuring...");
        lv_obj_set_style_text_color(lbl_status, lv_color_make(0xE2, 0x4B, 0x4A), 0);

        // Add to graph
        lv_chart_set_next_value(chart, ser, bpm);
        lv_chart_refresh(chart);

    } else if (fingerOn) {
        lv_label_set_text(lbl_bpm, "--");
        lv_label_set_text(lbl_status, "measuring...");
        lv_obj_set_style_text_color(lbl_status, lv_color_make(0x88, 0x88, 0x88), 0);

    } else {
        lv_label_set_text(lbl_bpm, "--");
        lv_obj_set_style_text_color(lbl_bpm, lv_color_make(0x55, 0x55, 0x55), 0);
        lv_label_set_text(lbl_status, "place finger on sensor");
        lv_obj_set_style_text_color(lbl_status, lv_color_make(0x55, 0x55, 0x55), 0);
    }
}

void hr_screen_show() {
    lv_scr_load_anim(scr_hr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

lv_obj_t* hr_screen_get() {
    return scr_hr;
}