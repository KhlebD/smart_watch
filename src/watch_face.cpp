#include "watch_face.h"
#include "globals.h"
#include "main_funcs.h"
#include "notif_screen.h"
#include "temp_screen.h"
#include "main_funcs.h"
#include <Arduino.h>

// ── Colors ────────────────────────────────────────────────────────────────
#define C_BG        lv_color_hex(0x080c14)
#define C_BPM       lv_color_hex(0xff4010)
#define C_STEPS     lv_color_hex(0x39ff14)
#define C_TEMP      lv_color_hex(0x00e5ff)
#define C_BLE_ON    lv_color_hex(0x00e5ff)
#define C_BLE_OFF   lv_color_hex(0x444444)
#define C_MUTED     lv_color_hex(0x2a3a4a)
#define C_DIM       lv_color_hex(0x666666)
#define C_WHITE     lv_color_hex(0xffffff)

// ── Widget handles ────────────────────────────────────────────────────────
static lv_obj_t *arc_bpm        = nullptr;
static lv_obj_t *arc_steps      = nullptr;
static lv_obj_t *arc_temp       = nullptr;

static lv_obj_t *lbl_time       = nullptr;
static lv_obj_t *lbl_weekday    = nullptr;
static lv_obj_t *lbl_date       = nullptr;
static lv_obj_t *lbl_battery    = nullptr;
static lv_obj_t *lbl_ble        = nullptr;

static lv_obj_t *lbl_bpm_title  = nullptr;
static lv_obj_t *lbl_bpm_val    = nullptr;
static lv_obj_t *lbl_steps_title = nullptr;
static lv_obj_t *lbl_steps_val  = nullptr;
static lv_obj_t *lbl_temp_title  = nullptr;
static lv_obj_t *lbl_temp_val   = nullptr;

// ── Arc factory ───────────────────────────────────────────────────────────
static lv_obj_t* make_arc(lv_obj_t* parent, int size, int width,
                           lv_color_t color, int bg_start, int bg_end) {
    lv_obj_t* arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);
    lv_arc_set_bg_angles(arc, bg_start, bg_end);
    lv_arc_set_angles(arc, bg_start, bg_start); // empty fill

    // Transparent body
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(arc, 0, LV_PART_MAIN);

    // Dim track
    lv_obj_set_style_arc_color(arc, color, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);

    // Bright indicator
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);

    // Hide knob
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(arc, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);

    return arc;
}

// ── Label factory ─────────────────────────────────────────────────────────
static lv_obj_t* make_label(lv_obj_t* parent, const char* text,
                              const lv_font_t* font, lv_color_t color) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_TRANSP, 0);
    return lbl;
}

// ── Gesture handler ───────────────────────────────────────────────────────
static void gesture_cb(lv_event_t* e) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    switch (dir) {
        case LV_DIR_BOTTOM:  notif_screen_show(LV_SCR_LOAD_ANIM_MOVE_BOTTOM); break;
        case LV_DIR_TOP:    show_temp_screen(LV_SCR_LOAD_ANIM_MOVE_TOP);     break;
        case LV_DIR_RIGHT: show_hr_screen(LV_SCR_LOAD_ANIM_MOVE_RIGHT);     break;
        case LV_DIR_LEFT:  show_step_screen(LV_SCR_LOAD_ANIM_MOVE_LEFT);    break;
        default: break;
    }
}
// ── Long Press for Music / Run ──────────────────────────────────────────────────
static void long_press_cb(lv_event_t* e) {
    if (run_active) show_run_screen(LV_SCR_LOAD_ANIM_FADE_IN);
    else            show_music_screen(LV_SCR_LOAD_ANIM_FADE_IN);
}
// ── Create ────────────────────────────────────────────────────────────────
void watch_face_create(lv_obj_t* screen) {
    lv_obj_set_style_bg_color(screen, C_BG, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(screen, gesture_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(screen, long_press_cb, LV_EVENT_LONG_PRESSED, NULL);

    // ── Arcs ──────────────────────────────────────────────────────────────
    // BPM: 9 o'clock (180°) CW to 3 o'clock (360°) — top half
    arc_bpm   = make_arc(screen, 238, 5, C_BPM,   180, 360);
    // Steps: 3 o'clock (0°) CW to 9 o'clock (180°) — bottom half
    arc_steps = make_arc(screen, 238, 5, C_STEPS,   0, 180);
    // Temp: small inner arc centered at 6 o'clock (90°) ±30°
    arc_temp  = make_arc(screen, 215, 2, C_TEMP,   60, 120);

    // ── Time (center) ─────────────────────────────────────────────────────
    lbl_time = make_label(screen, "00:00", &lv_font_montserrat_28, C_WHITE);
    lv_obj_align(lbl_time, LV_ALIGN_CENTER, 0, -8);

    // ── Weekday (below time) ──────────────────────────────────────────────
    lbl_weekday = make_label(screen, "MONDAY", &lv_font_montserrat_10, C_MUTED);
    lv_obj_align(lbl_weekday, LV_ALIGN_CENTER, 0, 18);

    // ── Date (top center) ─────────────────────────────────────────────────
    lbl_date = make_label(screen, "1 JAN", &lv_font_montserrat_14, C_WHITE);
    lv_obj_align(lbl_date, LV_ALIGN_TOP_MID, 0, 30);

    // ── Battery (below date, left) ────────────────────────────────────────
    lbl_battery = make_label(screen, "100%", &lv_font_montserrat_10, C_DIM);
    lv_obj_align(lbl_battery, LV_ALIGN_TOP_MID, -20, 48);

    // ── BLE symbol (below date, right) ───────────────────────────────────
    lbl_ble = make_label(screen, LV_SYMBOL_BLUETOOTH, &lv_font_montserrat_10, C_BLE_OFF);
    lv_obj_align(lbl_ble, LV_ALIGN_TOP_MID, 20, 48);

    // ── BPM (left side) ───────────────────────────────────────────────────
    lbl_bpm_title = make_label(screen, "BPM", &lv_font_montserrat_10, C_BPM);
    lv_obj_align(lbl_bpm_title, LV_ALIGN_LEFT_MID, 6, -10);
    lbl_bpm_val = make_label(screen, "--", &lv_font_montserrat_16, C_WHITE);
    lv_obj_align(lbl_bpm_val, LV_ALIGN_LEFT_MID, 6, 6);

    // ── Steps (right side) ────────────────────────────────────────────────
    lbl_steps_title = make_label(screen, "STEPS", &lv_font_montserrat_10, C_STEPS);
    lv_obj_set_style_text_align(lbl_steps_title, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(lbl_steps_title, LV_ALIGN_RIGHT_MID, -6, -10);
    lbl_steps_val = make_label(screen, "--", &lv_font_montserrat_16, C_WHITE);
    lv_obj_set_style_text_align(lbl_steps_val, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(lbl_steps_val, LV_ALIGN_RIGHT_MID, -6, 6);

    // ── Temp (bottom) ─────────────────────────────────────────────────────
    lbl_temp_title = make_label(screen, "TEMP", &lv_font_montserrat_10, C_TEMP);
    lv_obj_align(lbl_temp_title, LV_ALIGN_BOTTOM_MID, 0, -44);
    lbl_temp_val = make_label(screen, "--°", &lv_font_montserrat_16, C_WHITE);
    lv_obj_align(lbl_temp_val, LV_ALIGN_BOTTOM_MID, 0, -26);
}

// ── Update ────────────────────────────────────────────────────────────────
void watch_face_update(int hour, int min, int sec,
                        int battery_pct, bool ble_connected,
                        int day, int month, int weekday,
                        float temp, int bpm,
                        int steps, int step_goal) {

    static const char* WD[]  = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
    static const char* MON[] = {"JAN","FEB","MAR","APR","MAY","JUN",
                                 "JUL","AUG","SEP","OCT","NOV","DEC"};
    char buf[32];

    // Time
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, min);
    lv_label_set_text(lbl_time, buf);

    // Weekday
    if (weekday >= 0 && weekday <= 6)
        lv_label_set_text(lbl_weekday, WD[weekday]);

    // Date
    if (day > 0 && month >= 1 && month <= 12) {
        snprintf(buf, sizeof(buf), "%d %s", day, MON[month - 1]);
        lv_label_set_text(lbl_date, buf);
    }

    // Battery
    snprintf(buf, sizeof(buf), "%d%%", battery_pct);
    lv_label_set_text(lbl_battery, buf);

    // BLE
    lv_obj_set_style_text_color(lbl_ble,
        ble_connected ? C_BLE_ON : C_BLE_OFF, 0);

    // BPM arc — fills top half based on bpm/200
    {
        int fill = (bpm > 0) ? (int)((bpm / 200.0f) * 180.0f) : 0;
        fill = constrain(fill, 0, 180);
        lv_arc_set_angles(arc_bpm, 180, 180 + fill);
        if (bpm > 0) snprintf(buf, sizeof(buf), "%d", bpm);
        else         snprintf(buf, sizeof(buf), "--");
        lv_label_set_text(lbl_bpm_val, buf);
    }

    // Steps arc — fills bottom half based on steps/goal
    {
        int fill = (step_goal > 0)
            ? (int)((steps / (float)step_goal) * 180.0f)
            : 0;
        fill = constrain(fill, 0, 180);
        lv_arc_set_angles(arc_steps, 0, fill);
        snprintf(buf, sizeof(buf), "%d", steps);
        lv_label_set_text(lbl_steps_val, buf);
    }

    // Temp arc — spreads from center (90°) based on temp 0-40°C
    {
        float clamped = constrain(temp, 0.0f, 40.0f);
        int spread = (int)((clamped / 40.0f) * 30.0f);
        spread = max(spread, 1); // always show at least a dot
        lv_arc_set_angles(arc_temp, 90 - spread, 90 + spread);
        snprintf(buf, sizeof(buf), "%.0f\xc2\xb0", temp); // UTF-8 degree symbol
        lv_label_set_text(lbl_temp_val, buf);
    }

}
