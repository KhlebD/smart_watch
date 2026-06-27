#include "sleep.h"
#include "config.h"
#include <Arduino.h>
#include <esp_sleep.h>

#define SLEEP_DURATION_SECONDS 30

void configureSleepWakeSources() {
    // Touch wake — EXT0 on GPIO33, wakes when pin goes LOW
    esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_INT, 0);

    // Timer wake — every 30 seconds
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SECONDS * 1000000ULL);
}

int getWakeReason() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            return WAKE_REASON_TOUCH;
        case ESP_SLEEP_WAKEUP_TIMER:
            return WAKE_REASON_TIMER;
        default:
            return WAKE_REASON_BOOT;
    }
}

void goToSleep() {
    Serial.println("Going to sleep...");
    Serial.flush();

    // Turn off display backlight
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);

    delay(100);
    esp_deep_sleep_start();
}