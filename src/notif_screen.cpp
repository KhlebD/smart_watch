#include "notif_screen.h"
#include "globals.h"
#include "watch_face.h"
#include <stdio.h>
#include "lv_hebrew_fonts.h"
#include "main_funcs.h"

#define CARD_BG_COLOR  lv_color_make(0x22, 0x22, 0x22)
#define TEXT_PRIMARY   lv_color_white()
#define TEXT_SECONDARY lv_color_make(0xBD, 0xF7, 0xBD)
#define TEXT_DIM       lv_color_make(0x88, 0x88, 0x88)
#define RED            lv_color_make(0xE2, 0x4B, 0x4A)

static lv_obj_t *scr_list     = nullptr;
static lv_obj_t *scr_expanded = nullptr;
static lv_obj_t *list_cont    = nullptr;
static lv_obj_t *lbl_exp_src  = nullptr;
static lv_obj_t *lbl_exp_title= nullptr;
static lv_obj_t *lbl_exp_body = nullptr;
static int        expanded_idx = -1;

static lv_style_t sty_div;

// ── Forward declarations ──────────────────────────────────────────────────
static void build_list();
static void open_expanded(int idx);
static void dismiss_cb(lv_event_t *e);
static void card_tap_cb(lv_event_t *e);

// ── Helpers ───────────────────────────────────────────────────────────────
static String truncate(const String &s, int maxLen) {
    if ((int)s.length() <= maxLen) return s;
    return s.substring(0, maxLen - 3) + "...";
}

// ── Card tap callback ─────────────────────────────────────────────────────
static void card_tap_cb(lv_event_t *e) {
    lv_obj_t *card = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(card);
    open_expanded(idx);
}

// ── Dismiss callback ──────────────────────────────────────────────────────
static void dismiss_cb(lv_event_t *e) {
    if (expanded_idx >= 0 && expanded_idx < (int)notifQueue.size()) {
        notifQueue.erase(notifQueue.begin() + expanded_idx);
        expanded_idx = -1;
    }

    build_list();

    if (notifQueue.empty()) {
        show_watch_face();
    } else {
        lv_scr_load_anim(scr_list,
                         LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
    }
}

// ── Build list ────────────────────────────────────────────────────────────
static void build_list() {
    lv_obj_clean(list_cont);

    int total = notifQueue.size();

    if (total == 0) {
        lv_obj_t *empty = lv_label_create(list_cont);
        lv_label_set_text(empty, "all caught up");
        lv_obj_set_style_text_color(empty, TEXT_DIM, 0);
        lv_obj_set_style_text_font(empty, &lv_heb_ascii_14, 0);
        lv_obj_center(empty);
        return;
    }

    for (int i = 0; i < total; i++) {
        WatchNotification &n = notifQueue[i];

        // Card
        lv_obj_t *card = lv_obj_create(list_cont);
        lv_obj_set_width(card, 200);
        lv_obj_set_height(card, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(card, CARD_BG_COLOR, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_pad_all(card, 10, 0);
        lv_obj_set_style_pad_row(card, 4, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

        lv_obj_set_user_data(card, (void*)(intptr_t)i);
        lv_obj_add_event_cb(card, card_tap_cb, LV_EVENT_SHORT_CLICKED, nullptr);

        // App name
        lv_obj_t *lbl_src = lv_label_create(card);
        lv_label_set_text(lbl_src, truncate(n.src, 22).c_str());
        lv_obj_set_style_text_color(lbl_src, TEXT_DIM, 0);
        lv_obj_set_style_text_font(lbl_src, &lv_heb_ascii_14, 0);
        lv_obj_set_width(lbl_src, 180);
        lv_label_set_long_mode(lbl_src, LV_LABEL_LONG_CLIP);

        // Title
        lv_obj_t *lbl_title = lv_label_create(card);
        lv_label_set_text(lbl_title, truncate(n.title, 24).c_str());
        lv_obj_set_style_text_color(lbl_title, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(lbl_title, &lv_heb_ascii_16, 0);
        lv_obj_set_width(lbl_title, 180);
        lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_CLIP);

        // Body preview
        lv_obj_t *lbl_body = lv_label_create(card);
        lv_label_set_text(lbl_body, truncate(n.body, 30).c_str());
        lv_obj_set_style_text_color(lbl_body, TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(lbl_body, &lv_heb_ascii_14, 0);
        lv_obj_set_width(lbl_body, 180);
        lv_label_set_long_mode(lbl_body, LV_LABEL_LONG_CLIP);
    }
}

// ── Open expanded ─────────────────────────────────────────────────────────
static void open_expanded(int idx) {
    if (idx < 0 || idx >= (int)notifQueue.size()) return;
    expanded_idx = idx;

    WatchNotification &n = notifQueue[idx];

    lv_label_set_text(lbl_exp_src,   n.src.c_str());
    lv_label_set_text(lbl_exp_title, n.title.c_str());
    lv_label_set_text(lbl_exp_body,  truncate(n.body, 150).c_str());

    lv_scr_load_anim(scr_expanded,
                     LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

// ── Public API ────────────────────────────────────────────────────────────
void notif_screen_create() {

    // ── List screen ──────────────────────────────────────────────────────
    scr_list = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_list, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr_list, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_list, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t *hdr = lv_label_create(scr_list);
    lv_label_set_text(hdr, "notifications");
    lv_obj_set_style_text_color(hdr, TEXT_DIM, 0);
    lv_obj_set_style_text_font(hdr, &lv_heb_ascii_14, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 8);

    // Scrollable card container
    list_cont = lv_obj_create(scr_list);
    lv_obj_set_size(list_cont, 220, 200);
    lv_obj_align(list_cont, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_bg_opa(list_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_cont, 0, 0);
    lv_obj_set_style_pad_all(list_cont, 0, 0);
    lv_obj_set_style_pad_row(list_cont, 8, 0);
    lv_obj_add_flag(list_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(list_cont, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(list_cont, LV_SCROLL_SNAP_NONE);
    lv_obj_set_scrollbar_mode(list_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_cont, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // ── Expanded screen ──────────────────────────────────────────────────
    scr_expanded = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_expanded, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr_expanded, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_expanded, LV_OBJ_FLAG_SCROLLABLE);

    // App name
    lbl_exp_src = lv_label_create(scr_expanded);
    lv_obj_set_style_text_color(lbl_exp_src, TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl_exp_src, &lv_heb_ascii_14, 0);
    lv_obj_align(lbl_exp_src, LV_ALIGN_TOP_MID, 0, 10);

    // Divider
    static lv_point_t div_pts[2] = {{20, 0}, {220, 0}};
    lv_style_init(&sty_div);
    lv_style_set_line_color(&sty_div, lv_color_make(0x44, 0x44, 0x44));
    lv_style_set_line_width(&sty_div, 1);
    lv_obj_t *div = lv_line_create(scr_expanded);
    lv_obj_add_style(div, &sty_div, 0);
    lv_line_set_points(div, div_pts, 2);
    lv_obj_align(div, LV_ALIGN_TOP_MID, 0, 28);

    // Card background
    lv_obj_t *exp_card = lv_obj_create(scr_expanded);
    lv_obj_set_size(exp_card, 200, 155);
    lv_obj_align(exp_card, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_bg_color(exp_card, CARD_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(exp_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(exp_card, 0, 0);
    lv_obj_set_style_radius(exp_card, 8, 0);
    lv_obj_set_style_pad_all(exp_card, 10, 0);
    lv_obj_set_style_pad_row(exp_card, 6, 0);
    lv_obj_clear_flag(exp_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(exp_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(exp_card, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Title
    lbl_exp_title = lv_label_create(exp_card);
    lv_label_set_text(lbl_exp_title, "");
    lv_obj_set_style_text_color(lbl_exp_title, TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl_exp_title, &lv_heb_ascii_16, 0);
    lv_obj_set_width(lbl_exp_title, 180);
    lv_label_set_long_mode(lbl_exp_title, LV_LABEL_LONG_WRAP);

    // Body
    lbl_exp_body = lv_label_create(exp_card);
    lv_label_set_text(lbl_exp_body, "");
    lv_obj_set_style_text_color(lbl_exp_body, TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(lbl_exp_body, &lv_heb_ascii_14, 0);
    lv_obj_set_width(lbl_exp_body, 180);
    lv_label_set_long_mode(lbl_exp_body, LV_LABEL_LONG_WRAP);

    // Dismiss button
    lv_obj_t *btn = lv_btn_create(scr_expanded);
    lv_obj_set_size(btn, 140, 32);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn, lv_color_make(0x33, 0x33, 0x33), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_add_event_cb(btn, dismiss_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "dismiss");
    lv_obj_set_style_text_color(btn_lbl, TEXT_DIM, 0);
    lv_obj_set_style_text_font(btn_lbl, &lv_heb_ascii_14, 0);
    lv_obj_center(btn_lbl);
}

void notif_screen_refresh() {
    build_list();
}

void notif_screen_show() {
    build_list();
    lv_scr_load_anim(scr_list,
                     LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}