#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <RTClib.h>
#include <MAX30105.h>
#include <heartRate.h>
#include "config.h"
#include "globals.h"
#include "touch.h"
#include "ble_comm.h"
#include "battery.h"
#include "lvgl_port.h"
#include "watch_face.h"
#include "notif_screen.h"
#include "step_screen.h"
#include "hr_screen.h"
#include "motion.h"
#include "baro.h"
#include "main_funcs.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ── Display state ─────────────────────────────────────────────────────────
unsigned long screenOnTime = 0;

// ── Heart rate ────────────────────────────────────────────────────────────
MAX30105 particleSensor;
bool     hrSensorReady = false;

const byte RATE_SIZE = 4;
byte  hrRates[RATE_SIZE];
byte  hrRateSpot = 0;
long  hrLastBeat = 0;
int   hrBeatAvg  = 0;
long  hrIrValue  = 0;

// ── Public functions ──────────────────────────────────────────────────────
void show_watch_face(lv_scr_load_anim_t anim) {
    lv_scr_load_anim(scr_watch, anim, 200, 0, false);
}

void show_step_screen() {
    step_screen_update(motion_get_steps(), 10000, baro_get_altitude());
    step_screen_show();
}

void show_hr_screen() {
    hrBeatAvg  = 0;
    hrRateSpot = 0;
    memset(hrRates, 0, sizeof(hrRates));
    hr_screen_show();
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
    Serial2.begin(115200);
    delay(100);

    // ── Touch + I2C main bus ──────────────────────────────────────────────
    resetTouch();
    pinMode(TOUCH_INT, INPUT);
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(50000);
    configureTouch();
    setupTouchInterrupt(TOUCH_INT);

    // ── RTC ───────────────────────────────────────────────────────────────
    if (!rtc.begin()) {
        Serial2.println("RTC not found");
    }
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // uncomment once to set time

    // ── Motion sensor (BMI160) ────────────────────────────────────────────
    motion_init();

    // ── Barometric sensor (BMP280) ────────────────────────────────────────
    baro_init();

    // ── Heart rate (MAX30102) on Wire1 ────────────────────────────────────
    Wire1.begin(HR_SDA, HR_SCL);
    Wire1.setClock(400000);
    if (!particleSensor.begin(Wire1, I2C_SPEED_FAST)) {
        Serial2.println("MAX30102 not found");
    } else {
        particleSensor.setup();
        particleSensor.setPulseAmplitudeRed(0x0A);
        particleSensor.setPulseAmplitudeGreen(0);
        hrSensorReady = true;
        Serial2.println("MAX30102 ok");
    }

    // ── Display ───────────────────────────────────────────────────────────
    display.init();
    display.setRotation(0);
    display.invertDisplay(true);
    display.setBrightness(200);

    // ── LVGL ─────────────────────────────────────────────────────────────
    lvgl_port_init(display);

    // ── BLE ───────────────────────────────────────────────────────────────
    initBLE();

    // ── Screens ───────────────────────────────────────────────────────────
    scr_watch = lv_scr_act();
    watch_face_create(scr_watch);
    notif_screen_create();
    step_screen_create();
    hr_screen_create();

    screenOnTime = millis();
    Serial2.println("Setup complete");
}

// ── Loop ──────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── Touch ──────────────────────────────────────────────────────────────
    static bool wasDown = false;

    if (touchEventFlag || wasDown) {
        touchEventFlag = false;
        screenOnTime   = now;

        int  tx = 0, ty = 0;
        bool fingerDown = false;

        Wire.beginTransmission(TOUCH_ADDR);
        Wire.write(0x01);
        if (Wire.endTransmission(false) == 0 &&
            Wire.requestFrom(TOUCH_ADDR, 6) == 6) {
            Wire.read();
            int  points = Wire.read();
            byte xHigh  = Wire.read();
            byte xLow   = Wire.read();
            byte yHigh  = Wire.read();
            byte yLow   = Wire.read();
            if (points > 0) {
                fingerDown = true;
                tx = ((xHigh & 0x0F) << 8) | xLow;
                ty = ((yHigh & 0x0F) << 8) | yLow;
            }
        }

        lastTouchX    = tx;
        lastTouchY    = ty;
        lastTouchDown = fingerDown;
        wasDown       = fingerDown;
    }

    // ── Pending notification ────────────────────────────────────────────────
    if (pendingNotification) {
        pendingNotification = false;
        notif_screen_show();
        screenOnTime = now;
    }

    // ── Heart rate ──────────────────────────────────────────────────────────
    if (hrSensorReady && lv_scr_act() == hr_screen_get()) {
        hrIrValue = particleSensor.getIR();
        if (checkForBeat(hrIrValue)) {
            long delta = millis() - hrLastBeat;
            hrLastBeat = millis();
            float bpm  = 60 / (delta / 1000.0);
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

    // ── Update watch face every second ──────────────────────────────────────
    static unsigned long lastUpdate = 0;
    if (now - lastUpdate >= 1000) {
        lastUpdate = now;
        DateTime t = rtc.now();
        if (t.hour() < 24 && t.minute() < 60 && t.second() < 60) {
            watch_face_update(
                t.hour(),
                t.minute(),
                t.second(),
                getBatteryPercent(),
                bleConnected,
                t.day(),
                t.month(),
                baro_get_temperature()
            );
        } else {
            Serial2.println("Bad RTC read — recovering bus");
            recoverI2C();
            rtc.begin();
        }
    }

    // ── Update step screen every 5 seconds ─────────────────────────────────
    static unsigned long lastStepUpdate = 0;
    if (now - lastStepUpdate >= 5000) {
        lastStepUpdate = now;
        if (lv_scr_act() == step_screen_get()) {
            step_screen_update(motion_get_steps(), 10000, baro_get_altitude());
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
        lastDraw     = 0;
    }

    // ── LVGL ───────────────────────────────────────────────────────────────
    lv_timer_handler();
    delay(5);
}