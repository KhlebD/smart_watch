#include <Arduino.h>
#include <MAX30105.h>
#include <RTClib.h>
#include <Wire.h>
#include <heartRate.h>
#include <lvgl.h>

#include "baro.h"
#include "battery.h"
#include "ble_comm.h"
#include "config.h"
#include "globals.h"
#include "hr_screen.h"
#include "lvgl_port.h"
#include "main_funcs.h"
#include "motion.h"
#include "notif_screen.h"
#include "step_screen.h"
#include "touch.h"
#include "watch_face.h"

static lv_obj_t* lbl_touch = nullptr;
unsigned long screenOnTime = 0;

MAX30105 particleSensor;
bool hrSensorReady = false;
const byte RATE_SIZE = 4;
byte hrRates[RATE_SIZE];
byte hrRateSpot = 0;
long hrLastBeat = 0;
int hrBeatAvg = 0;
long hrIrValue = 0;

void show_watch_face(lv_scr_load_anim_t anim) {
    lv_scr_load_anim(scr_watch, anim, 200, 0, false);
}

void show_step_screen() {
    step_screen_update(motion_get_steps(), 10000, baro_get_altitude());
    step_screen_show();
}

void show_hr_screen() {
    hrBeatAvg = 0;
    hrRateSpot = 0;
    memset(hrRates, 0, sizeof(hrRates));
    hr_screen_show();
}

void setup() {
    Serial.begin(115200);
    delay(100);

    // ── Touch + I2C ───────────────────────────────────────────────────────
    resetTouch();
    pinMode(TOUCH_INT, INPUT);
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    // Barometer
    baro_init();
    // Motion
    motion_init();
    // Heart Rate
    Wire1.begin(13, 14);
    Wire1.setClock(400000);
    if (!particleSensor.begin(Wire1, I2C_SPEED_FAST)) {
        Serial.println("MAX30102 not found");
    } else {
        particleSensor.setup();
        particleSensor.setPulseAmplitudeRed(0x0A);
        particleSensor.setPulseAmplitudeGreen(0);
        hrSensorReady = true;
        Serial.println("MAX30102 ok");
    }

    Wire.setClock(50000);
    configureTouch();
    setupTouchInterrupt(TOUCH_INT);

    // ── Display ───────────────────────────────────────────────────────────
    display.init();
    display.setRotation(0);
    display.invertDisplay(true);
    display.setBrightness(200);

    // ── LVGL ─────────────────────────────────────────────────────────────
    lvgl_port_init(display);

    // ── BLE _─────────────────────────────────────────────────────────────

    initBLE();

    // ── Watch face only ───────────────────────────────────────────────────
    scr_watch = lv_scr_act();
    watch_face_create(scr_watch);

    // REST OF SCREENS
    notif_screen_create();
    step_screen_create();
    hr_screen_create();
    screenOnTime = millis();
    Serial.println("Setup complete");
}

void loop() {
    unsigned long now = millis();

    // ── Touch — only read when INT pin is low ─────────────────────────────
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

    // ── Heart Rate ───────────────────────────────────────────
    if (hrSensorReady && lv_scr_act() == hr_screen_get()) {
        hrIrValue = particleSensor.getIR();
        if (checkForBeat(hrIrValue)) {
            long delta = millis() - hrLastBeat;
            hrLastBeat = millis();
            float bpm = 60 / (delta / 1000.0);
            if (bpm > 20 && bpm < 255) {
                hrRates[hrRateSpot++] = (byte)bpm;
                hrRateSpot %= RATE_SIZE;
                hrBeatAvg = 0;
                for (byte x = 0; x < RATE_SIZE; x++) hrBeatAvg += hrRates[x];
                hrBeatAvg /= RATE_SIZE;
            }
        }
        static unsigned long lastHrUpdate = 0;
        if (now - lastHrUpdate >= 1000) {
            lastHrUpdate = now;
            hr_screen_update(hrBeatAvg, hrIrValue);
        }
    }
    // ── Update watch face every second with fake time ───────────────────────
    static unsigned long lastUpdate = 0;
    if (now - lastUpdate >= 1000) {
        lastUpdate = now;
        unsigned long s = now / 1000;
        watch_face_update((s / 3600) % 12, (s / 60) % 60, s % 60, 75,
                          bleConnected, 19, 8, baro_get_temperature());
    }

    if (pendingNotification) {
        pendingNotification = false;
        notif_screen_show();
        screenOnTime = now;
    }
    // ── Display timeout ─────────────────────────────────────────────────────
    if (now - screenOnTime > DISPLAY_TIMEOUT_MS) {
        display.setBrightness(0);
        while (!touchEventFlag) {
            lv_timer_handler();
            delay(10);
        }
        touchEventFlag = false;
        display.setBrightness(200);
        display.setBrightness(200);
        lv_obj_invalidate(lv_scr_act());
        lv_timer_handler();
        screenOnTime = millis();
        lastDraw = 0;
    }
    // ── LVGL ───────────────────────────────────────────────────────────────
    lv_timer_handler();
    delay(5);
}