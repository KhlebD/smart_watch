#include <Arduino.h>
#include "battery.h"
#include "config.h"

int getBatteryPercent() {
    int raw = analogRead(BATTERY_PIN);
    float voltage = (raw / 4095.0) * 3.3 * 2;
    int percent = (int)((voltage - 3.0) / (4.2 - 3.0) * 100);
    return constrain(percent, 0, 100);
}