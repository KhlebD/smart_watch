#include "sleep.h"
#include "config.h"
#include "globals.h"
#include <Arduino.h>
#include <esp_sleep.h>

#define SLEEP_DURATION_SECONDS 30

void goToLightSleep() {
    Serial.println("Light sleep...");
    Serial.flush();
    touchEventFlag = false;
    // keep BLE on
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_INT, 0);
    esp_light_sleep_start();
    // Returns here on wake
    Serial.println("Woke from light sleep");
    touchEventFlag = false;
    delay(200);
    // Caller handles display
}