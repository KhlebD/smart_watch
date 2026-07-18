#include "watch_face.h"

#include <math.h>
#include <stdio.h>

#include "main_funcs.h"

#define CX 120
#define CY 120

// ── Persistent point arrays ───────────────────────────────────────────────
static lv_point_t pts_hour[2];
static lv_point_t pts_minute[2];
static lv_point_t pts_second[2];
static lv_point_t pts_tail[2];
static lv_point_t tick_pts[12][2];

// ── Styles ────────────────────────────────────────────────────────────────
static lv_style_t sty_hour;
static lv_style_t sty_minute;
static lv_style_t sty_second;
static lv_style_t sty_major_tick;
static lv_style_t sty_minor_tick;

// ── Widget handles ────────────────────────────────────────────────────────
static lv_obj_t* hand_hour;
static lv_obj_t* hand_minute;
static lv_obj_t* hand_second;
static lv_obj_t* hand_tail;
static lv_obj_t* lbl_date;
static lv_obj_t* lbl_battery;
static lv_obj_t* lbl_ble;
static lv_obj_t* lbl_temp;

// ── Math helper ───────────────────────────────────────────────────────────
static void calc_point(float angle_deg, int length, lv_point_t* out) {
    float rad = (angle_deg - 90.0f) * (float)M_PI / 180.0f;
    out->x = (lv_coord_t)(CX + roundf(length * cosf(rad)));
    out->y = (lv_coord_t)(CY + roundf(length * sinf(rad)));
}

// ── Create ────────────────────────────────────────────────────────────────
void watch_face_create(lv_obj_t* scr) {
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Tick styles
    lv_style_init(&sty_major_tick);
    lv_style_set_line_color(&sty_major_tick, lv_color_white());
    lv_style_set_line_width(&sty_major_tick, 3);
    lv_style_set_line_rounded(&sty_major_tick, true);

    lv_style_init(&sty_minor_tick);
    lv_style_set_line_color(&sty_minor_tick, lv_color_white());
    lv_style_set_line_width(&sty_minor_tick, 1);
    lv_style_set_line_rounded(&sty_minor_tick, true);

    // Draw 12 tick marks — create first so hands render on top
    for (int i = 0; i < 12; i++) {
        float angle = i * 30.0f;
        bool major = (i % 3 == 0);
        int outer_r = 108;
        int inner_r = major ? 90 : 100;

        calc_point(angle, outer_r, &tick_pts[i][0]);
        calc_point(angle, inner_r, &tick_pts[i][1]);

        lv_obj_t* tick = lv_line_create(scr);
        lv_obj_add_style(tick, major ? &sty_major_tick : &sty_minor_tick, 0);
        lv_line_set_points(tick, tick_pts[i], 2);
    }

    // Hand styles
    lv_style_init(&sty_hour);
    lv_style_set_line_color(&sty_hour, lv_color_white());
    lv_style_set_line_width(&sty_hour, 5);
    lv_style_set_line_rounded(&sty_hour, true);

    lv_style_init(&sty_minute);
    lv_style_set_line_color(&sty_minute, lv_color_white());
    lv_style_set_line_width(&sty_minute, 3);
    lv_style_set_line_rounded(&sty_minute, true);

    lv_style_init(&sty_second);
    lv_style_set_line_color(&sty_second, lv_color_make(0xE2, 0x4B, 0x4A));
    lv_style_set_line_width(&sty_second, 1);
    lv_style_set_line_rounded(&sty_second, true);

    // Hour hand
    pts_hour[0] = {CX, CY};
    pts_hour[1] = {CX, CY - 55};
    hand_hour = lv_line_create(scr);
    lv_obj_add_style(hand_hour, &sty_hour, 0);
    lv_line_set_points(hand_hour, pts_hour, 2);

    // Minute hand
    pts_minute[0] = {CX, CY};
    pts_minute[1] = {CX, CY - 75};
    hand_minute = lv_line_create(scr);
    lv_obj_add_style(hand_minute, &sty_minute, 0);
    lv_line_set_points(hand_minute, pts_minute, 2);

    // Second hand
    pts_second[0] = {CX, CY};
    pts_second[1] = {CX, CY - 85};
    hand_second = lv_line_create(scr);
    lv_obj_add_style(hand_second, &sty_second, 0);
    lv_line_set_points(hand_second, pts_second, 2);

    // Second tail (counterbalance)
    pts_tail[0] = {CX, CY};
    pts_tail[1] = {CX, CY + 20};
    hand_tail = lv_line_create(scr);
    lv_obj_add_style(hand_tail, &sty_second, 0);
    lv_line_set_points(hand_tail, pts_tail, 2);

    // Center dot — white outer
    lv_obj_t* dot_white = lv_obj_create(scr);
    lv_obj_set_size(dot_white, 10, 10);
    lv_obj_set_style_radius(dot_white, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot_white, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(dot_white, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot_white, 0, 0);
    lv_obj_set_style_pad_all(dot_white, 0, 0);
    lv_obj_align(dot_white, LV_ALIGN_CENTER, 0, 0);

    // Center dot — red inner
    lv_obj_t* dot_red = lv_obj_create(scr);
    lv_obj_set_size(dot_red, 5, 5);
    lv_obj_set_style_radius(dot_red, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot_red, lv_color_make(0xE2, 0x4B, 0x4A), 0);
    lv_obj_set_style_bg_opa(dot_red, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot_red, 0, 0);
    lv_obj_set_style_pad_all(dot_red, 0, 0);
    lv_obj_align(dot_red, LV_ALIGN_CENTER, 0, 0);

    // BLE icon
    lbl_ble = lv_label_create(scr);
    lv_obj_set_style_text_color(lbl_ble, lv_color_make(0x44, 0x88, 0xFF), 0);
    lv_obj_set_style_text_font(lbl_ble, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl_ble, LV_SYMBOL_BLUETOOTH);
    lv_obj_align(lbl_ble, LV_ALIGN_TOP_MID, -18, 36);

    // Battery label
    lbl_battery = lv_label_create(scr);
    lv_obj_set_style_text_color(lbl_battery, lv_color_make(0x88, 0x88, 0x88),
                                0);
    lv_obj_set_style_text_font(lbl_battery, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl_battery, "");
    lv_obj_align_to(lbl_battery, lbl_ble, LV_ALIGN_OUT_RIGHT_MID, 4, 0);

    // Date label
    lbl_date = lv_label_create(scr);
    lv_obj_set_style_text_color(lbl_date, lv_color_make(0x88, 0x88, 0x88), 0);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl_date, "");
    lv_obj_align(lbl_date, LV_ALIGN_BOTTOM_MID, 0, -42);

    // Temperature label
    lbl_temp = lv_label_create(scr);
    lv_obj_set_style_text_color(lbl_temp, lv_color_make(0x88, 0x88, 0x88), 0);
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl_temp, "");
    lv_obj_align(lbl_temp, LV_ALIGN_TOP_MID, 32, 36);

    // Swipe up → step screen, swipe down → heart rate screen
    lv_obj_add_event_cb(
        scr,
        [](lv_event_t* e) {
            lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
            if (dir == LV_DIR_TOP) {
                extern void show_step_screen();
                show_step_screen();
            } else if (dir == LV_DIR_BOTTOM) {
                extern void show_hr_screen();
                show_hr_screen();
            }
        },
        LV_EVENT_GESTURE, nullptr);
}

// ── Update ────────────────────────────────────────────────────────────────
void watch_face_update(int hours, int minutes, int seconds, int battery,
                       bool ble_connected, int day, int month,
                       float temperature) {
    // Angles
    float hour_angle = (hours % 12) * 30.0f + minutes * 0.5f;
    float minute_angle = minutes * 6.0f + seconds * 0.1f;
    float second_angle = seconds * 6.0f;

    // Hour hand
    calc_point(hour_angle, 55, &pts_hour[1]);
    lv_line_set_points(hand_hour, pts_hour, 2);

    // Minute hand
    calc_point(minute_angle, 75, &pts_minute[1]);
    lv_line_set_points(hand_minute, pts_minute, 2);

    // Second hand + tail
    calc_point(second_angle, 85, &pts_second[1]);
    lv_line_set_points(hand_second, pts_second, 2);
    calc_point(second_angle + 180.0f, 20, &pts_tail[1]);
    lv_line_set_points(hand_tail, pts_tail, 2);

    // Battery
    char bat_buf[8];
    snprintf(bat_buf, sizeof(bat_buf), "%d%%", battery);
    lv_label_set_text(lbl_battery, bat_buf);
    lv_obj_set_style_text_color(lbl_battery,
                                battery > 20 ? lv_color_make(0x88, 0x88, 0x88)
                                             : lv_color_make(0xE2, 0x4B, 0x4A),
                                0);

    // BLE
    lv_obj_set_style_text_color(lbl_ble,
                                ble_connected ? lv_color_make(0x44, 0x88, 0xFF)
                                              : lv_color_make(0x44, 0x44, 0x44),
                                0);

    // Date
    char date_buf[8];
    snprintf(date_buf, sizeof(date_buf), "%02d/%02d", day, month);
    lv_label_set_text(lbl_date, date_buf);

    // Temperature
    char tempBuf[10];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f°", temperature);
    lv_label_set_text(lbl_temp, tempBuf);
}