#include "lvgl_port.h"
#include "config.h"
#include "globals.h"
#include <esp_heap_caps.h>

static LGFX* _display = nullptr;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;

static void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    _display->startWrite();
    _display->setAddrWindow(area->x1, area->y1, w, h);
    _display->writePixels((lgfx::rgb565_t*)color_p, w * h);
    _display->endWrite();
    lv_disp_flush_ready(disp);
}

static void touch_read(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    if (lastTouchDown) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = lastTouchX;
        data->point.y = lastTouchY;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void lvgl_port_init(LGFX &display) {
    _display = &display;

    lv_init();

    // Allocate draw buffers in PSRAM
    buf1 = (lv_color_t*)heap_caps_malloc(
        240 * 20 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t*)heap_caps_malloc(
        240 * 20 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (!buf1 || !buf2) {
        Serial.println("PSRAM alloc failed — falling back to internal RAM");
        static lv_color_t fallback1[240 * 10];
        static lv_color_t fallback2[240 * 10];
        lv_disp_draw_buf_init(&draw_buf, fallback1, fallback2, 240 * 10);
    } else {
        Serial.println("LVGL buffers in PSRAM");
        lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 240 * 20);
    }

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = 240;
    disp_drv.ver_res  = 240;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_drv_register(&indev_drv);

    Serial.println("LVGL v8 port initialized");
}