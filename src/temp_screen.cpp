#include "temp_screen.h"
#include "main_funcs.h"
#include <Arduino.h>

// ── Widget handles ────────────────────────────────────────────────────────
static lv_obj_t* scr_temp    = nullptr;
static lv_obj_t* lbl_city    = nullptr;
static lv_obj_t* lbl_temp    = nullptr;
static lv_obj_t* lbl_alt     = nullptr;
static lv_obj_t* sun_glow    = nullptr;
static lv_obj_t* sun_body    = nullptr;
static lv_obj_t* moon_body   = nullptr;
static lv_obj_t* moon_cut    = nullptr;

// ── Sky color ─────────────────────────────────────────────────────────────
static lv_color_t get_sky_color(int hour, int minute) {
    // Before BLE sync, default to day
    if (hour == 0 && minute == 0) return lv_color_make(100, 181, 246);

    if (hour >= 20 || hour < 6) {
        return lv_color_make(8, 12, 30);          // night
    } else if (hour < 18) {
        return lv_color_make(100, 181, 246);       // day blue
    } else {
        // 18:00-20:00 — fade to dark
        float t = constrain(((hour - 18) * 60 + minute) / 120.0f, 0.0f, 1.0f);
        return lv_color_make(
            (uint8_t)(100 * (1.0f - t) + 8  * t),
            (uint8_t)(181 * (1.0f - t) + 12 * t),
            (uint8_t)(246 * (1.0f - t) + 30 * t)
        );
    }
}

// ── Gesture: swipe down → watch face ─────────────────────────────────────
static void gesture_cb(lv_event_t* e) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;
    if (lv_indev_get_gesture_dir(indev) == LV_DIR_BOTTOM) {
        show_watch_face(LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
    }
}

// ── Helper: make a circle object ──────────────────────────────────────────
static lv_obj_t* make_circle(lv_obj_t* parent, int size, lv_color_t color, lv_opa_t opa) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

// ── Create ────────────────────────────────────────────────────────────────
void temp_screen_create() {
    scr_temp = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_temp, lv_color_make(100, 181, 246), 0);
    lv_obj_set_style_bg_opa(scr_temp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr_temp, 0, 0);
    lv_obj_clear_flag(scr_temp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr_temp, gesture_cb, LV_EVENT_GESTURE, NULL);

    // ── Sun ───────────────────────────────────────────────────────────────
    sun_glow = make_circle(scr_temp, 70, lv_color_make(255, 210, 0), LV_OPA_30);
    lv_obj_align(sun_glow, LV_ALIGN_CENTER, -44, -15);

    sun_body = make_circle(scr_temp, 46, lv_color_make(255, 210, 0), LV_OPA_COVER);
    lv_obj_align(sun_body, LV_ALIGN_CENTER, -44, -15);

    // ── Moon (hidden by default) ──────────────────────────────────────────
    moon_body = make_circle(scr_temp, 46, lv_color_make(220, 220, 255), LV_OPA_COVER);
    lv_obj_align(moon_body, LV_ALIGN_CENTER, -44, -15);
    lv_obj_add_flag(moon_body, LV_OBJ_FLAG_HIDDEN);

    // Cutout offset to create crescent shape
    moon_cut = make_circle(scr_temp, 38, lv_color_make(8, 12, 30), LV_OPA_COVER);
    lv_obj_align(moon_cut, LV_ALIGN_CENTER, -36, -20);
    lv_obj_add_flag(moon_cut, LV_OBJ_FLAG_HIDDEN);

    // ── City label ────────────────────────────────────────────────────────
    lbl_city = lv_label_create(scr_temp);
    lv_label_set_text(lbl_city, "Jerusalem");
    lv_obj_set_style_text_color(lbl_city, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_city, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_city, LV_ALIGN_TOP_MID, 0, 32);

    // ── Temperature label ─────────────────────────────────────────────────
    lbl_temp = lv_label_create(scr_temp);
    lv_label_set_text(lbl_temp, "--\xc2\xb0");
    lv_obj_set_style_text_color(lbl_temp, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_28, 0);
    lv_obj_align(lbl_temp, LV_ALIGN_CENTER, 36, -15);

    // ── Altitude label ────────────────────────────────────────────────────
    lbl_alt = lv_label_create(scr_temp);
    lv_label_set_text(lbl_alt, "Alt: --m");
    lv_obj_set_style_text_color(lbl_alt, lv_color_make(220, 240, 255), 0);
    lv_obj_set_style_text_font(lbl_alt, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_alt, LV_ALIGN_BOTTOM_MID, 0, -32);
}

// ── Show ──────────────────────────────────────────────────────────────────
void temp_screen_show() {
    lv_scr_load_anim(scr_temp, LV_SCR_LOAD_ANIM_MOVE_TOP, 200, 0, false);
}

lv_obj_t* temp_screen_get() {
    return scr_temp;
}

// ── Update ────────────────────────────────────────────────────────────────
void temp_screen_update(float temp, float altitude, int hour, int minute) {
    // Background
    lv_color_t sky = get_sky_color(hour, minute);
    lv_obj_set_style_bg_color(scr_temp, sky, 0);

    // Sun / Moon
    bool is_day = (hour == 0 && minute == 0) || (hour >= 6 && hour < 20);
    if (is_day) {
        lv_obj_clear_flag(sun_glow,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sun_body,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(moon_body,   LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(moon_cut,    LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(sun_glow,    LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sun_body,    LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(moon_body, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(moon_cut,  LV_OBJ_FLAG_HIDDEN);
        // Cutout matches sky so crescent works during dusk too
        lv_obj_set_style_bg_color(moon_cut, sky, 0);
    }

    // Temperature
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f\xc2\xb0", temp);
    lv_label_set_text(lbl_temp, buf);

    // Altitude
    snprintf(buf, sizeof(buf), "Alt: %.0fm", altitude);
    lv_label_set_text(lbl_alt, buf);
}