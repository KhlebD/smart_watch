#include "music_screen.h"

#include <Arduino.h>

#include "ble_comm.h"
#include "globals.h"
#include "main_funcs.h"
#include "notif_screen.h"

#define C_GREEN lv_color_make(0x19, 0x9c, 0x47)
#define C_WHITE lv_color_white()
#define C_DARK lv_color_make(0x1a, 0x1a, 0x1a)

static lv_obj_t* scr_music = nullptr;
static lv_obj_t* arc_progress = nullptr;
static lv_obj_t* lbl_track = nullptr;
static lv_obj_t* lbl_artist = nullptr;
static lv_obj_t* lbl_time = nullptr;
static lv_obj_t* lbl_play_sym = nullptr;
static lv_obj_t* bar_vol = nullptr;
static lv_obj_t* lbl_vol = nullptr;

// ── Gesture: up → temp, down → notifications ─────────────────────────────
static void gesture_cb(lv_event_t* e) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_BOTTOM) show_temp_screen(LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
    if (dir == LV_DIR_TOP) notif_screen_show(LV_SCR_LOAD_ANIM_MOVE_TOP);
}
// ── Button callbacks ──────────────────────────────────────────────────────
static void btn_prev_cb(lv_event_t* e) { send_music_cmd("prev"); }
static void btn_next_cb(lv_event_t* e) { send_music_cmd("next"); }

static void btn_play_cb(lv_event_t* e) {
    if (music_state == "playing") {
        send_music_cmd("pause");
        music_state = "paused";
        lv_label_set_text(lbl_play_sym, LV_SYMBOL_PAUSE);
        lv_obj_align(lbl_play_sym, LV_ALIGN_CENTER, 0, 0);  // centered
    } else {
        send_music_cmd("play");
        music_state = "playing";
        lv_label_set_text(lbl_play_sym, LV_SYMBOL_PLAY);
        lv_obj_align(lbl_play_sym, LV_ALIGN_CENTER, 2, 0);  // shifted right
    }
}

static void btn_voldown_cb(lv_event_t* e) {
    send_music_cmd("voldown");
    music_vol = max(0, music_vol - 10);
    int bar_w = max(1, (int)(60.0f * music_vol / 100.0f));
    lv_obj_set_width(bar_vol, bar_w);
    lv_obj_align(bar_vol, LV_ALIGN_BOTTOM_MID, (bar_w - 60) / 2, -51);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", music_vol);
    lv_label_set_text(lbl_vol, buf);
}

static void btn_volup_cb(lv_event_t* e) {
    send_music_cmd("volup");
    music_vol = min(100, music_vol + 10);
    int bar_w = max(1, (int)(60.0f * music_vol / 100.0f));
    lv_obj_set_width(bar_vol, bar_w);
    lv_obj_align(bar_vol, LV_ALIGN_BOTTOM_MID, (bar_w - 60) / 2, -51);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", music_vol);
    lv_label_set_text(lbl_vol, buf);
}

static lv_obj_t* make_btn(lv_obj_t* parent, int size, lv_color_t color,
                          lv_event_cb_t cb) {
    lv_obj_t* btn = lv_obj_create(parent);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    return btn;
}

static lv_obj_t* make_sym(lv_obj_t* parent, const char* sym,
                          const lv_font_t* font, lv_color_t color,
                          int x_offset = 0) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, sym);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, x_offset, 0);
    return lbl;
}

void music_screen_create() {
    scr_music = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_music, lv_color_make(6, 6, 6), 0);
    lv_obj_set_style_bg_opa(scr_music, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_music, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr_music, gesture_cb, LV_EVENT_GESTURE, nullptr);

    // ── Progress arc ──────────────────────────────────────────────────────
    arc_progress = lv_arc_create(scr_music);
    lv_obj_set_size(arc_progress, 234, 234);
    lv_obj_align(arc_progress, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(arc_progress, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_rotation(arc_progress, 270);
    lv_arc_set_bg_angles(arc_progress, 0, 360);
    lv_arc_set_angles(arc_progress, 0, 0);
    lv_obj_set_style_bg_opa(arc_progress, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(arc_progress, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_progress, lv_color_make(0x1a, 0x1a, 0x1a),
                               LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_progress, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_progress, C_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_progress, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_progress, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc_progress, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(arc_progress, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc_progress, 0, LV_PART_KNOB);

    // ── Track / artist / time ─────────────────────────────────────────────
    lbl_track = lv_label_create(scr_music);
    lv_label_set_text(lbl_track, "Not playing");
    lv_label_set_long_mode(lbl_track, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl_track, 190);
    lv_obj_set_style_text_color(lbl_track, C_WHITE, 0);
    lv_obj_set_style_text_font(lbl_track, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(lbl_track, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_track, LV_ALIGN_TOP_MID, 0, 44);

    lbl_artist = lv_label_create(scr_music);
    lv_label_set_text(lbl_artist, "");
    lv_label_set_long_mode(lbl_artist, LV_LABEL_LONG_DOT);
    lv_obj_set_width(lbl_artist, 190);
    lv_obj_set_style_text_color(lbl_artist, C_WHITE, 0);
    lv_obj_set_style_text_font(lbl_artist, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_align(lbl_artist, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_artist, LV_ALIGN_TOP_MID, 0, 64);

    lbl_time = lv_label_create(scr_music);
    lv_label_set_text(lbl_time, "0:00 / 0:00");
    lv_obj_set_style_text_color(lbl_time, C_WHITE, 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_align(lbl_time, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_time, LV_ALIGN_TOP_MID, 0, 80);

    // ── Playback controls ─────────────────────────────────────────────────
    lv_obj_t* btn_prev = make_btn(scr_music, 44, C_DARK, btn_prev_cb);
    lv_obj_align(btn_prev, LV_ALIGN_CENTER, -62, 0);
    make_sym(btn_prev, LV_SYMBOL_PREV, &lv_font_montserrat_16, C_WHITE);

    lv_obj_t* btn_play = make_btn(scr_music, 58, C_GREEN, btn_play_cb);
    lv_obj_align(btn_play, LV_ALIGN_CENTER, 0, 0);
    // Black triangle, 3px to the right so it looks optically centered
    lbl_play_sym = make_sym(btn_play, LV_SYMBOL_PLAY, &lv_font_montserrat_20,
                            lv_color_black(), 3);

    lv_obj_t* btn_next = make_btn(scr_music, 44, C_DARK, btn_next_cb);
    lv_obj_align(btn_next, LV_ALIGN_CENTER, 62, 0);
    make_sym(btn_next, LV_SYMBOL_NEXT, &lv_font_montserrat_16, C_WHITE);

    // ── Volume section ────────────────────────────────────────────────────
    lv_obj_t* btn_vm = make_btn(scr_music, 28, C_DARK, btn_voldown_cb);
    lv_obj_align(btn_vm, LV_ALIGN_BOTTOM_MID, -48, -40);
    make_sym(btn_vm, LV_SYMBOL_MINUS, &lv_font_montserrat_14, C_WHITE);

    lv_obj_t* bar_track = lv_obj_create(scr_music);
    lv_obj_set_size(bar_track, 60, 4);
    lv_obj_align(bar_track, LV_ALIGN_BOTTOM_MID, 0, -51);
    lv_obj_set_style_bg_color(bar_track, lv_color_make(0x22, 0x22, 0x22), 0);
    lv_obj_set_style_bg_opa(bar_track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar_track, 0, 0);
    lv_obj_set_style_radius(bar_track, 2, 0);
    lv_obj_clear_flag(bar_track, LV_OBJ_FLAG_SCROLLABLE);

    bar_vol = lv_obj_create(scr_music);
    lv_obj_set_size(bar_vol, 42, 4);
    lv_obj_align(bar_vol, LV_ALIGN_BOTTOM_MID, -9, -51);
    lv_obj_set_style_bg_color(bar_vol, C_GREEN, 0);
    lv_obj_set_style_bg_opa(bar_vol, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar_vol, 0, 0);
    lv_obj_set_style_radius(bar_vol, 2, 0);

    lv_obj_t* btn_vp = make_btn(scr_music, 28, C_DARK, btn_volup_cb);
    lv_obj_align(btn_vp, LV_ALIGN_BOTTOM_MID, 48, -40);
    make_sym(btn_vp, LV_SYMBOL_PLUS, &lv_font_montserrat_14, C_WHITE);

    lbl_vol = lv_label_create(scr_music);
    lv_label_set_text(lbl_vol, "70%");
    lv_obj_set_style_text_color(lbl_vol, C_WHITE, 0);
    lv_obj_set_style_text_font(lbl_vol, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_vol, LV_ALIGN_BOTTOM_MID, 0, -24);
}

void music_screen_show() {
    lv_scr_load_anim(scr_music, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
}
lv_obj_t* music_screen_get() { return scr_music; }

void music_screen_update() {
    if (!scr_music) return;
    lv_label_set_text(lbl_track, music_track.c_str());
    lv_label_set_text(lbl_artist, music_artist.c_str());

    int cur_pos = music_pos;
    if (music_state == "playing" && music_pos_updated_at > 0) {
        cur_pos = music_pos + (int)((millis() - music_pos_updated_at) / 1000);
        if (cur_pos > music_dur) cur_pos = music_dur;
    }
    int deg = (music_dur > 0) ? (int)((float)cur_pos / music_dur * 360.0f) : 0;
    lv_arc_set_angles(arc_progress, 0, deg);

    char buf[24];
    snprintf(buf, sizeof(buf), "%d:%02d / %d:%02d", cur_pos / 60, cur_pos % 60,
             music_dur / 60, music_dur % 60);
    lv_label_set_text(lbl_time, buf);

    if (music_state == "playing") {
        lv_label_set_text(lbl_play_sym, LV_SYMBOL_PAUSE);
        lv_obj_align(lbl_play_sym, LV_ALIGN_CENTER, 0, 0);
    } else {
        lv_label_set_text(lbl_play_sym, LV_SYMBOL_PLAY);
        lv_obj_align(lbl_play_sym, LV_ALIGN_CENTER, 2, 0);
    }

    int bar_w = max(1, (int)(60.0f * music_vol / 100.0f));
    lv_obj_set_width(bar_vol, bar_w);
    lv_obj_align(bar_vol, LV_ALIGN_BOTTOM_MID, (bar_w - 60) / 2, -51);
    snprintf(buf, sizeof(buf), "%d%%", music_vol);
    lv_label_set_text(lbl_vol, buf);
}