#include <Arduino.h>
#include <MAX30105.h>
#include <Wire.h>
#include <heartRate.h>
#include <lvgl.h>
#include <time.h>

#include "baro.h"
#include "battery.h"
#include "ble_comm.h"
#include "config.h"
#include "esp_private/brownout.h"
#include "globals.h"
#include "hr_screen.h"
#include "lvgl_port.h"
#include "main_funcs.h"
#include "motion.h"
#include "music_screen.h"
#include "notif_screen.h"
#include "step_screen.h"
#include "temp_screen.h"
#include "touch.h"
#include "watch_face.h"

unsigned long screenOnTime = 0;

// ── HR vars ───────────────────────────────────────────────────────────────
MAX30105 particleSensor;
bool hrSensorReady = false;

const byte RATE_SIZE = 4;
byte hrRates[RATE_SIZE];
byte hrRateSpot = 0;
long hrLastBeat = 0;
int hrBeatAvg = 0;
long hrIrValue = 0;
static int hr_history[GRAPH_POINTS];
static int hr_history_head = 0;
static bool hr_history_full = false;

// ── Screen functions ──────────────────────────────────────────────────────
void show_watch_face(lv_scr_load_anim_t anim) {
    lv_scr_load_anim(scr_watch, anim, 200, 0, false);
}

void show_step_screen(lv_scr_load_anim_t anim) {
    step_screen_update(motion_get_steps(), 10000, baro_get_altitude());
    lv_scr_load_anim(step_screen_get(), anim, 200, 0, false);
}

void show_hr_screen(lv_scr_load_anim_t anim) {
    hr_screen_load_history(hr_history, GRAPH_POINTS, hr_history_head, hr_history_full);
    lv_scr_load_anim(hr_screen_get(), anim, 200, 0, false);
}

void show_temp_screen(lv_scr_load_anim_t anim) {
    temp_screen_update(baro_get_temperature(), baro_get_altitude(), g_hour, g_minute);
    lv_scr_load_anim(temp_screen_get(), anim, 200, 0, false);
}

void show_music_screen(lv_scr_load_anim_t anim) {
    music_screen_update();
    lv_scr_load_anim(music_screen_get(), anim, 200, 0, false);
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    esp_brownout_disable();

    Serial.begin(115200);
    delay(100);

    // ── Timezone ───────────────────────────────────────────────────────────
    setenv("TZ", "IST-2IDT,M3.4.4/26,M10.5.0", 1);
    tzset();

    // ── Touch + I2C bus 1 ─────────────────────────────────────────────────
    resetTouch();
    pinMode(TOUCH_INT, INPUT);
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(50000);
    configureTouch();
    setupTouchInterrupt(TOUCH_INT);

    // ── Barometric sensor (BMP280) ────────────────────────────────────────
    baro_init();

    // ── Motion sensor (BMI160) ────────────────────────────────────────────
    motion_init();

    // ── Heart rate (MAX30102) on Wire1 ────────────────────────────────────
    Wire1.begin(HR_SDA, HR_SCL);
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

    // ── Display ────────────────────────────────────────────────────────────
    display.init();
    display.setRotation(0);
    display.invertDisplay(true);
    display.setBrightness(200);

    // ── LVGL ──────────────────────────────────────────────────────────────
    lvgl_port_init(display);

    // ── BLE ────────────────────────────────────────────────────────────────
    initBLE();

    // ── Screens ────────────────────────────────────────────────────────────
    scr_watch = lv_scr_act();
    watch_face_create(scr_watch);
    notif_screen_create();
    step_screen_create();
    hr_screen_create();
    temp_screen_create();
    music_screen_create();

    screenOnTime = millis();
    Serial.println("Setup complete");
}

// ── Loop ──────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── Touch ──────────────────────────────────────────────────────────────
    static bool wasDown = false;
    static unsigned long lastSuccessfulRead = 0;
    static unsigned long lastUpdate = 0;
    static int lastDay = -1;

    if (touchEventFlag || (wasDown && (now - lastSuccessfulRead < 200))) {
        touchEventFlag = false;
        screenOnTime = now;

        int tx = 0, ty = 0;
        bool fingerDown = false;

        Wire.beginTransmission(TOUCH_ADDR);
        Wire.write(0x01);
        byte err = Wire.endTransmission(false);
        if (err == 0) {
            if (Wire.requestFrom(TOUCH_ADDR, 6) == 6) {
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
                    lastSuccessfulRead = now;
                }
            }
        } else {
            Wire.begin(TOUCH_SDA, TOUCH_SCL);
            Wire.setClock(50000);
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

    // ── Heart rate — always reading ────────────────────────────────────────
    if (hrSensorReady) {
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

                // Store in ring buffer
                hr_history[hr_history_head] = hrBeatAvg;
                hr_history_head = (hr_history_head + 1) % GRAPH_POINTS;
                if (hr_history_head == 0) hr_history_full = true;
            }
        }
        if (lv_scr_act() == hr_screen_get()) {
            static unsigned long lastHrUpdate = 0;
            if (now - lastHrUpdate >= 1000) {
                lastHrUpdate = now;
                hr_screen_update(hrBeatAvg, hrIrValue);
            }
        }
    }
    // ── Update watch face every second ──────────────────────────────────────

    if (now - lastUpdate >= 1000) {
        lastUpdate = now;

        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 0)) {
            g_hour = timeinfo.tm_hour;
            g_minute = timeinfo.tm_min;

            watch_face_update(
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                getBatteryPercent(), bleConnected, timeinfo.tm_mday,
                timeinfo.tm_mon + 1, timeinfo.tm_wday, baro_get_temperature(),
                hrBeatAvg, motion_get_steps(), 10000);

            // Midnight reset
            if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 &&
                timeinfo.tm_mday != lastDay) {
                lastDay = timeinfo.tm_mday;
                Wire.beginTransmission(0x69);
                Wire.write(0x7E);
                Wire.write(0xB2);
                Wire.endTransmission();
                Serial.println("Daily step count reset");
                hr_screen_reset_calories();
            }

        } else {
            // No BLE sync yet — seconds ticking
            unsigned long s = now / 1000;
            watch_face_update(0, 0, s % 60, getBatteryPercent(), bleConnected,
                              0, 0, 0, baro_get_temperature(), hrBeatAvg,
                              motion_get_steps(), 10000);
        }
    }

    // ── Update step screen every 5 seconds
    // ─────────────────────────────────
    static unsigned long lastStepUpdate = 0;
    if (now - lastStepUpdate >= 5000) {
        lastStepUpdate = now;
        if (lv_scr_act() == step_screen_get()) {
            step_screen_update(motion_get_steps(), 10000, baro_get_altitude());
        }
        if (lv_scr_act() == temp_screen_get()) {
            temp_screen_update(baro_get_temperature(), baro_get_altitude(),
                               g_hour, g_minute);
        }
        if (lv_scr_act() == music_screen_get()) {
            music_screen_update();
        }
    }

    // ── Display timeout
    // ─────────────────────────────────────────────────────
    if (now - screenOnTime > DISPLAY_TIMEOUT_MS) {
        display.setBrightness(0);
        motion_sleep_start();

        while (!touchEventFlag && !motion_get_flag()) {
            lv_timer_handler();
            delay(10);
        }

        motion_sleep_end();
        touchEventFlag = false;
        display.setBrightness(200);
        lv_obj_invalidate(lv_scr_act());
        lv_timer_handler();
        screenOnTime = millis();
        lastDraw = 0;
    }
    // ── LVGL ─────────────────────────────────────────────────────
    lv_timer_handler();
    delay(5);
}