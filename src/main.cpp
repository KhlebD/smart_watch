#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <lvgl.h>

#include "baro.h"
#include "battery.h"
#include "ble_comm.h"
#include "config.h"
#include "globals.h"
#include "lvgl_port.h"
#include "motion.h"
#include "notif_screen.h"
#include "step_screen.h"
#include "touch.h"
#include "watch_face.h"

// ── Display state ─────────────────────────────────────────────────────────
unsigned long screenOnTime = 0;

// ── Show watch face ───────────────────────────────────────────────────────
void show_watch_face(lv_scr_load_anim_t anim) {
    lv_scr_load_anim(scr_watch, anim, 200, 0, false);
}
// ── Show steps screen ───────────────────────────────────────────────────────
void show_step_screen() {
    step_screen_update(motion_get_steps(), 10000, baro_get_altitude());
    step_screen_show();
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(100);

    // Touch
    resetTouch();
    pinMode(TOUCH_INT, INPUT);
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    motion_init();
    baro_init();
    Wire.setClock(50000);

    configureTouch();
    setupTouchInterrupt(TOUCH_INT);

    // Display
    display.init();
    display.setRotation(0);
    display.invertDisplay(true);
    display.setBrightness(200);

    // LVGL
    lvgl_port_init(display);

    // Steps
    step_screen_create();

    // RTC
    if (!rtc.begin()) {
        Serial.println("RTC not found");
    }
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // uncomment once to set
    //  time

    // BLE
    initBLE();

    // Watch face screen
    scr_watch = lv_scr_act();
    watch_face_create(scr_watch);

    // Notification screens
    notif_screen_create();

    screenOnTime = millis();
    Serial.println("Setup complete");
}

void loop() {
    unsigned long now = millis();

    // ── Touch ──────────────────────────────────────────────────────────────
    static bool wasDown = false;

    if (touchEventFlag || wasDown) {
        touchEventFlag = false;
        screenOnTime = now;

        int tx = 0, ty = 0;
        bool fingerDown = false;

        Wire.beginTransmission(TOUCH_ADDR);
        Wire.write(0x01);
        if (Wire.endTransmission(false) == 0 &&
            Wire.requestFrom(TOUCH_ADDR, 6) == 6) {
            Wire.read();
            int points = Wire.read();
            byte xHigh = Wire.read();
            byte xLow = Wire.read();
            byte yHigh = Wire.read();
            byte yLow = Wire.read();
            if (points > 0) {
                fingerDown = true;
                tx = ((xHigh & 0x0F) << 8) | xLow;
                ty = ((yHigh & 0x0F) << 8) | yLow;
            }
        }

        lastTouchX = tx;
        lastTouchY = ty;
        lastTouchDown = fingerDown;
        wasDown = fingerDown;
    }

    // ── Pending notification ────────────────────────────────────────────────
    if (pendingNotification) {
        pendingNotification = false;
        notif_screen_show();
        screenOnTime = now;
    }

    // ── Update watch face every second ──────────────────────────────────────
    static unsigned long lastUpdate = 0;
    if (now - lastUpdate >= 1000) {
        lastUpdate = now;
        DateTime t = rtc.now();

        if (t.hour() < 24 && t.minute() < 60 && t.second() < 60) {
            watch_face_update(t.hour(), t.minute(), t.second(),
                              getBatteryPercent(), bleConnected, t.day(),
                              t.month(), baro_get_temperature());
        } else {
            Serial.println("Bad RTC read — recovering bus");
            recoverI2C();
            // Reinit devices after recovery
            if (!rtc.begin()) {
                Serial.println("RTC reinit failed");
            }
        }
    }

    // ── Display timeout ─────────────────────────────────────────────────────
    if (now - screenOnTime > DISPLAY_TIMEOUT_MS) {
        display.setBrightness(0);
        motion_sleep_start();

        while (!touchEventFlag && !motion_get_flag()) {
            lv_timer_handler();
            delay(10);
        }

        motion_sleep_end();
        touchEventFlag = false;
        display.init();
        display.setBrightness(200);
        lv_obj_invalidate(lv_scr_act());
        lv_timer_handler();
        screenOnTime = millis();
        lastDraw = 0;
    }
    static unsigned long lastStepUpdate = 0;
    if (now - lastStepUpdate >= 5000) {
        lastStepUpdate = now;
        // Only update if step screen is visible
        if (lv_scr_act() == step_screen_get()) {
            step_screen_update(motion_get_steps(), 10000, baro_get_altitude());
        }
    }
    // ── LVGL ───────────────────────────────────────────────────────────────
    lv_timer_handler();
    delay(5);
}