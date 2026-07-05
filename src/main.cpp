#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <lvgl.h>

#include "battery.h"
#include "ble_comm.h"
#include "config.h"
#include "globals.h"
#include "lvgl_port.h"
#include "notif_screen.h"
#include "touch.h"
#include "watch_face.h"
// Forward declared in notif_screen.cpp
void show_watch_face() {
    extern lv_obj_t* scr_watch;
    lv_scr_load_anim(scr_watch, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

void setup() {
    Serial.begin(115200);
    delay(100);

    resetTouch();
    pinMode(TOUCH_INT, INPUT);
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(100000);
    configureTouch();
    setupTouchInterrupt(TOUCH_INT);

    display.init();
    display.setRotation(0);
    display.invertDisplay(true);
    display.setBrightness(200);

    lvgl_port_init(display);

    // RTC
    if (!rtc.begin()) {
        Serial.println("RTC not found");
    }
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // uncomment once to set
    //  time Create watch face

    // BLE
    initBLE();

    lv_obj_t* scr = lv_scr_act();
    watch_face_create(scr);

    Serial.println("Watch face ready");

    // Store watch screen reference
    extern lv_obj_t* scr_watch;
    scr_watch = lv_scr_act();
    watch_face_create(scr_watch);

    // Create notification screens
    notif_screen_create();
}

void loop() {
    // Handle pending notification
    if (pendingNotification) {
        pendingNotification = false;
        notif_screen_show();
    }

    // Touch
    static unsigned long lastTouchRead = 0;
    if (millis() - lastTouchRead >= 20) {
        lastTouchRead = millis();

        int tx, ty;
        bool fingerDown = false;

        // Direct poll — works for both taps and drags
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
    }
    // Update watch face every second
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();
        DateTime now = rtc.now();
        watch_face_update(now.hour(), now.minute(), now.second(),
                          getBatteryPercent(), bleConnected, now.day(),
                          now.month());
    }

    lv_timer_handler();
    delay(5);
}