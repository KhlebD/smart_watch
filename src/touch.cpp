#include <Arduino.h>
#include <Wire.h>
#include "touch.h"
#include "config.h"

void resetTouch() {
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(10);
    digitalWrite(TOUCH_RST, HIGH);
    delay(50);
}
void configureTouch() {
    // Set CST816S to continuous touch report mode
    // Register 0xFA = IRQ control, value 0x41 = enable continuous active
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0xFA);
    Wire.write(0x41);
    Wire.endTransmission();

    // Register 0xEC = motion mask, enable swipe detection
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0xEC);
    Wire.write(0x01);
    Wire.endTransmission();

    Serial.println("Touch configured for continuous mode");
}
void recoverI2C() {
    pinMode(TOUCH_SDA, OUTPUT);
    pinMode(TOUCH_SCL, OUTPUT);
    digitalWrite(TOUCH_SDA, HIGH);

    for (int i = 0; i < 9; i++) {
        digitalWrite(TOUCH_SCL, HIGH);
        delayMicroseconds(5);
        digitalWrite(TOUCH_SCL, LOW);
        delayMicroseconds(5);
    }

    // Generate STOP condition
    digitalWrite(TOUCH_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(TOUCH_SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(TOUCH_SDA, HIGH);
    delayMicroseconds(5);

    Serial.println("I2C bus recovered");
}
static bool readRaw(int &x, int &y, int &points) {
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0x01);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(TOUCH_ADDR, 6) != 6) return false;
    Wire.read();
    points    = Wire.read();
    byte xHigh = Wire.read();
    byte xLow  = Wire.read();
    byte yHigh = Wire.read();
    byte yLow  = Wire.read();
    x = ((xHigh & 0x0F) << 8) | xLow;
    y = ((yHigh & 0x0F) << 8) | yLow;
    return true;
}

bool readTouch(int &x, int &y) {
    if (digitalRead(TOUCH_INT) == HIGH) return false;
    int points;
    if (!readRaw(x, y, points)) return false;
    return points > 0;
}

int readGesture() {
    if (digitalRead(TOUCH_INT) == HIGH) return GESTURE_NONE;
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0x01);
    if (Wire.endTransmission(false) != 0) return GESTURE_NONE;
    if (Wire.requestFrom(TOUCH_ADDR, 6) != 6) return GESTURE_NONE;
    int gesture = Wire.read();
    for (int i = 0; i < 5; i++) Wire.read();
    return gesture;
}

bool readTouchRaw(TouchEvent &evt) {
    bool fingerDown = digitalRead(TOUCH_INT) == LOW;
    int x, y, points;
    if (fingerDown && readRaw(x, y, points) && points > 0) {
        evt.x      = x;
        evt.y      = y;
        evt.active = true;
        return true;
    }
    evt.active = false;
    return false;
}