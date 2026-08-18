#include "baro.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

static Adafruit_BMP280 bmp;
static bool initialized = false;

void baro_init() {
    if (!bmp.begin(0x77)) {
        Serial2.println("BMP280 not found");
        return;
    }
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    initialized = true;
    Serial2.println("BMP280 initialized");
}

float baro_get_altitude() {
    if (!initialized) return 0.0f;
    return bmp.readAltitude(1013.25);
}

float baro_get_temperature() {
    if (!initialized) return 0.0f;
    return bmp.readTemperature();
}

float baro_get_pressure() {
    if (!initialized) return 0.0f;
    return bmp.readPressure() / 100.0f;
}