#include "temp_screen.h"
#include "main_funcs.h"
#include <Arduino.h>
#include <math.h>
#include <esp_heap_caps.h>

// ── Widget handles ────────────────────────────────────────────────────────
static lv_obj_t* scr_temp     = nullptr;
static lv_obj_t* lbl_city     = nullptr;
static lv_obj_t* lbl_temp     = nullptr;
static lv_obj_t* lbl_altitude = nullptr;
static lv_obj_t* canvas_sun   = nullptr;

#define SUN_SIZE 80
static uint8_t* sun_buf = nullptr;

// ── Sky color based on hour ───────────────────────────────────────────────
static lv_color_t get_sky_color(int hour, int minute) {
    if (hour >= 20 || hour < 6) {
        // Night — dark navy
        return lv_color_make(8, 12, 30);
    } else if (hour < 18) {
        // Full day — sky blue
        return lv_color_make(100, 181, 246);
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

// ── Draw sun onto canvas ──────────────────────────────────────────────────
static void draw_sun() {
    if (!canvas_sun || !sun_buf) return;

    // Clear to transparent
    lv_canvas_fill_bg(canvas_sun, lv_color_black(), LV_OPA_TRANSP);

    int cx = SUN_SIZE / 2;
    int cy = SUN_SIZE / 2;

    // Rays (drawn first so circle covers inner ends)
    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.color     = lv_color_make(255, 210, 0);
    ld.width     = 4;
    ld.opa       = LV_OPA_COVER;
    ld.round_start = 1;
    ld.round_end   = 1;

    for (int i = 0; i < 8; i++) {
        float a = i * (float)M_PI / 4.0f;
        lv_point_t pts[2] = {
            { (lv_coord_t)(cx + (int)(23.0f * cosf(a))),
              (lv_coord_t)(cy + (int)(23.0f * sinf(a))) },
            { (lv_coord_t)(cx + (int)(37.0f * cosf(a))),
              (lv_coord_t)(cy + (int)(37.0f * sinf(a))) }
        };
        lv_canvas_draw_line(canvas_sun, pts, 2, &ld);
    }

    // Sun body — filled circle
    lv_draw_rect_dsc_t rd;
    lv_draw_rect_dsc_init(&rd);
    rd.bg_color    = lv_color_make(255, 210, 0);
    rd.bg_opa      = LV_OPA_COVER;
    rd.radius      = LV_RADIUS_CIRCLE;
    rd.border_width = 0;
    // Center a 36×36 circle on the canvas
    lv_canvas_draw_rect(canvas_sun, cx - 18, cy - 18, 36, 36, &rd);
}

// ── Gesture: swipe down → back to watch face ─────────────────────────────
static void gesture_cb(lv_event_t* e) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;
    if (lv_indev_get_gesture_dir(indev) == LV_DIR_BOTTOM) {
        show_watch_face(LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
    }
}

// ── Create ────────────────────────────────────────────────────────────────
void temp_screen_create() {
    scr_temp = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_temp, lv_color_make(100, 181, 246), 0);
    lv_obj_set_style_bg_opa(scr_temp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr_temp, 0, 0);
    lv_obj_clear_flag(scr_temp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr_temp, gesture_cb, LV_EVENT_GESTURE, NULL);

    // City label
    lbl_city = lv_label_create(scr_temp);
    lv_label_set_text(lbl_city, "Jerusalem");
    lv_obj_set_style_text_color(lbl_city, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_city, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_city, LV_ALIGN_TOP_MID, 0, 32);

    // Sun canvas (PSRAM allocated)
    sun_buf = (uint8_t*)heap_caps_malloc(SUN_SIZE * SUN_SIZE * 4, MALLOC_CAP_SPIRAM);
    if (sun_buf) {
        canvas_sun = lv_canvas_create(scr_temp);
        lv_canvas_set_buffer(canvas_sun, sun_buf, SUN_SIZE, SUN_SIZE,
                             LV_IMG_CF_TRUE_COLOR_ALPHA);
        lv_obj_set_style_bg_opa(canvas_sun, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(canvas_sun, 0, 0);
        lv_obj_align(canvas_sun, LV_ALIGN_CENTER, -44, -15);
        draw_sun();
    }

    // Temperature label (large)
    lbl_temp = lv_label_create(scr_temp);
    lv_label_set_text(lbl_temp, "--\xc2\xb0");
    lv_obj_set_style_text_color(lbl_temp, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_28, 0);
    lv_obj_align(lbl_temp, LV_ALIGN_CENTER, 36, -15);

    // Altitude label (bottom)
    lbl_altitude = lv_label_create(scr_temp);
    lv_label_set_text(lbl_altitude, "Alt: --m");
    lv_obj_set_style_text_color(lbl_altitude, lv_color_make(220, 240, 255), 0);
    lv_obj_set_style_text_font(lbl_altitude, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_altitude, LV_ALIGN_BOTTOM_MID, 0, -32);
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
    // Background color
    lv_color_t sky = get_sky_color(hour, minute);
    lv_obj_set_style_bg_color(scr_temp, sky, 0);

    // Sun visibility
    if (canvas_sun) {
        bool is_day = (hour >= 6 && hour < 20);
        if (is_day) lv_obj_clear_flag(canvas_sun, LV_OBJ_FLAG_HIDDEN);
        else        lv_obj_add_flag(canvas_sun, LV_OBJ_FLAG_HIDDEN);
    }

    // Temperature
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f\xc2\xb0", temp);
    lv_label_set_text(lbl_temp, buf);

    // Altitude
    snprintf(buf, sizeof(buf), "Alt: %.0fm", altitude);
    lv_label_set_text(lbl_altitude, buf);
}