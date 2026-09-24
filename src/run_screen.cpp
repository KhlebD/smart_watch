#include "run_screen.h"
#include "main_funcs.h"
#include "globals.h"
#include "ble_comm.h"
#include <Arduino.h>

#define C_RED   lv_color_make(0xff, 0x40, 0x10)
#define C_GREEN lv_color_make(0x39, 0xff, 0x14)
#define C_WHITE lv_color_white()
#define C_DIM   lv_color_make(0x66, 0x66, 0x66)

static lv_obj_t* scr_run      = nullptr;
static lv_obj_t* lbl_distance = nullptr;
static lv_obj_t* lbl_time     = nullptr;
static lv_obj_t* lbl_pace     = nullptr;
static lv_obj_t* lbl_speed    = nullptr;
static lv_obj_t* lbl_hr       = nullptr;
static lv_obj_t* lbl_state    = nullptr;
static lv_obj_t* arc_progress = nullptr;


// Add near top of run_screen.cpp
static void swipe_cb(lv_event_t* e) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_RIGHT) show_step_screen(LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    if (dir == LV_DIR_LEFT)  show_hr_screen(LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

// ── Button callbacks ──────────────────────────────────────────────────────
static void btn_pause_cb(lv_event_t* e) {
    if (run_state == "running") send_run_cmd("pause");
    else                        send_run_cmd("resume");
}
static void btn_stop_cb(lv_event_t* e) {
    send_run_cmd("stop");
}

static lv_obj_t* make_stat(lv_obj_t* parent, const char* label,
                             lv_color_t color, lv_coord_t x, lv_coord_t y) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, x, y);
    
    return lbl;
}

// ── Create ────────────────────────────────────────────────────────────────
void run_screen_create() {
    scr_run = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_run, lv_color_make(6, 6, 6), 0);
    lv_obj_set_style_bg_opa(scr_run, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_run, LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_add_event_cb(scr_run, gesture_cb, LV_EVENT_GESTURE, nullptr);
    lv_obj_add_event_cb(scr_run, swipe_cb, LV_EVENT_GESTURE, nullptr);
    // ── Outer progress arc (time-based) ──────────────────────────────────
    arc_progress = lv_arc_create(scr_run);
    lv_obj_set_size(arc_progress, 234, 234);
    lv_obj_align(arc_progress, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(arc_progress, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_rotation(arc_progress, 270);
    lv_arc_set_bg_angles(arc_progress, 0, 360);
    lv_arc_set_angles(arc_progress, 0, 0);
    lv_obj_set_style_bg_opa(arc_progress, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(arc_progress, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_progress, lv_color_make(0x1a,0x1a,0x1a), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_progress, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_progress, C_RED, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_progress, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_progress, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc_progress, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(arc_progress, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc_progress, 0, LV_PART_KNOB);

    // ── State label (top) ─────────────────────────────────────────────────
    lbl_state = lv_label_create(scr_run);
    lv_label_set_text(lbl_state, "READY");
    lv_obj_set_style_text_color(lbl_state, C_DIM, 0);
    lv_obj_set_style_text_font(lbl_state, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_state, LV_ALIGN_TOP_MID, 0, 28);

    // ── Distance (large, center) ──────────────────────────────────────────
    lbl_distance = lv_label_create(scr_run);
    lv_label_set_text(lbl_distance, "0.00");
    lv_obj_set_style_text_color(lbl_distance, C_WHITE, 0);
    lv_obj_set_style_text_font(lbl_distance, &lv_font_montserrat_28, 0);
    lv_obj_align(lbl_distance, LV_ALIGN_CENTER, 0, -52);

    lv_obj_t* lbl_km = lv_label_create(scr_run);
    lv_label_set_text(lbl_km, "km");
    lv_obj_set_style_text_color(lbl_km, C_RED, 0);
    lv_obj_set_style_text_font(lbl_km, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_km, LV_ALIGN_CENTER, 0, -28);

    // ── Time ──────────────────────────────────────────────────────────────
    lbl_time = make_stat(scr_run, "0:00:00", C_WHITE, 0, -10);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_16, 0);

    // ── Pace / Speed / HR row ─────────────────────────────────────────────
    lbl_pace  = make_stat(scr_run, "--:--\n/km",   C_WHITE, -62, 14);
    lbl_speed = make_stat(scr_run, "--.-\nkm/h",   C_WHITE,   0, 14);
    lbl_hr    = make_stat(scr_run, "-- \nbpm",     C_RED,    62, 14);

    // ── Pause / Stop buttons ──────────────────────────────────────────────
    lv_obj_t* btn_pause = lv_obj_create(scr_run);
    lv_obj_set_size(btn_pause, 80, 32);
    lv_obj_align(btn_pause, LV_ALIGN_BOTTOM_MID, -50, -48);
    lv_obj_set_style_bg_color(btn_pause, lv_color_make(0x1a,0x1a,0x1a), 0);
    lv_obj_set_style_border_width(btn_pause, 0, 0);
    lv_obj_set_style_radius(btn_pause, 16, 0);
    lv_obj_clear_flag(btn_pause, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn_pause, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(btn_pause, btn_pause_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_pause = lv_label_create(btn_pause);
    lv_label_set_text(lbl_pause, "Pause");
    lv_obj_set_style_text_color(lbl_pause, C_WHITE, 0);
    lv_obj_set_style_text_font(lbl_pause, &lv_font_montserrat_10, 0);
    lv_obj_center(lbl_pause);

    lv_obj_t* btn_stop = lv_obj_create(scr_run);
    lv_obj_set_size(btn_stop, 80, 32);
    lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_MID, 50, -48);
    lv_obj_set_style_bg_color(btn_stop, lv_color_make(0x33,0x00,0x00), 0);
    lv_obj_set_style_border_width(btn_stop, 0, 0);
    lv_obj_set_style_radius(btn_stop, 16, 0);
    lv_obj_clear_flag(btn_stop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn_stop, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(btn_stop, btn_stop_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_stop = lv_label_create(btn_stop);
    lv_label_set_text(lbl_stop, "Stop");
    lv_obj_set_style_text_color(lbl_stop, C_RED, 0);
    lv_obj_set_style_text_font(lbl_stop, &lv_font_montserrat_10, 0);
    lv_obj_center(lbl_stop);
}

// ── Show ──────────────────────────────────────────────────────────────────
void run_screen_show() {
    lv_scr_load_anim(scr_run, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

lv_obj_t* run_screen_get() { return scr_run; }

// ── Update ────────────────────────────────────────────────────────────────
void run_screen_update(float distKm, long timeSec, float paceMinKm,
                       float speedKmh, int hr, const char* state) {
    if (!scr_run) return;
    char buf[32];

    snprintf(buf, sizeof(buf), "%.2f", distKm);
    lv_label_set_text(lbl_distance, buf);

    snprintf(buf, sizeof(buf), "%ld:%02ld:%02ld",
             timeSec/3600, (timeSec%3600)/60, timeSec%60);
    lv_label_set_text(lbl_time, buf);

    if (paceMinKm > 0) {
        snprintf(buf, sizeof(buf), "%d:%02d\n/km",
                 (int)paceMinKm, (int)((paceMinKm-(int)paceMinKm)*60));
    } else {
        snprintf(buf, sizeof(buf), "--:--\n/km");
    }
    lv_label_set_text(lbl_pace, buf);

    snprintf(buf, sizeof(buf), "%.1f\nkm/h", speedKmh);
    lv_label_set_text(lbl_speed, buf);

    snprintf(buf, sizeof(buf), "%d\nbpm", hr);
    lv_label_set_text(lbl_hr, buf);

    lv_label_set_text(lbl_state, state);

    // Arc fills proportionally to a 30-minute target run
    int deg = (int)((timeSec / 1800.0f) * 360.0f);
    if (deg > 360) deg = 360;
    lv_arc_set_angles(arc_progress, 0, deg);
}
