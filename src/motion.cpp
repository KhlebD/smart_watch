#include "motion.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"

#define BMI160_ADDR 0x69
#define REG_CMD 0x7E
#define REG_ACC_CONF 0x40
#define REG_ACC_RANGE 0x41
#define REG_INT_EN_0 0x50
#define REG_INT_OUT_CTRL 0x53
#define REG_INT_MAP_0 0x55
#define REG_INT_LOWHIGH 0x5A
#define REG_INT_MOTION 0x5C
#define REG_DATA_14 0x12
#define CMD_ACC_NORMAL 0x11
#define REG_STEP_CNT_0  0x78
#define REG_STEP_CNT_1  0x79
#define REG_STEP_CONF_0 0x7A
#define REG_STEP_CONF_1 0x7B
#define CMD_STEP_CLR    0xB2

static volatile bool motionFlag = false;

static void IRAM_ATTR motionISR() { motionFlag = true; }

static void writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(BMI160_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

void motion_init() {
    Serial2.println("BMI160 init start");
    writeReg(REG_CMD, 0xB6);
    delay(100);
    Serial2.println("Reset done");

    writeReg(REG_CMD, CMD_ACC_NORMAL);
    delay(100);
    Serial2.println("Accel awake");

    writeReg(REG_ACC_CONF, 0x28);
    writeReg(REG_ACC_RANGE, 0x05);
    delay(10);
    Serial2.println("Accel configured");

    writeReg(REG_STEP_CONF_0, 0x2D);
    writeReg(REG_STEP_CONF_1, 0x08);
    writeReg(REG_CMD, CMD_STEP_CLR);
    delay(10);
    Serial2.println("Step counter enabled");

    writeReg(REG_INT_LOWHIGH + 2, 0xA0);
    writeReg(REG_INT_MOTION, 0x01);
    writeReg(REG_INT_EN_0, 0x07);
    writeReg(REG_INT_OUT_CTRL, 0x0A);
    writeReg(REG_INT_MAP_0, 0x04);
    delay(10);
    Serial2.println("Interrupts configured");

    pinMode(BMI160_INT1, INPUT);
    Serial2.println("Pin mode set");

    attachInterrupt(digitalPinToInterrupt(BMI160_INT1), motionISR, RISING);
    Serial2.println("BMI160 initialized with interrupt");
}

bool motion_detected() {
    Wire.beginTransmission(BMI160_ADDR);
    Wire.write(REG_DATA_14);
    Wire.endTransmission(false);
    Wire.requestFrom(BMI160_ADDR, 6);

    if (Wire.available() < 6) return false;

    int16_t ax = Wire.read() | (Wire.read() << 8);
    int16_t ay = Wire.read() | (Wire.read() << 8);
    int16_t az = Wire.read() | (Wire.read() << 8);

    long magnitude = (long)abs(ax) + abs(ay) + abs(az);
    long motion = abs(magnitude - 8192);

    return motion > 4500;
}

bool motion_get_flag() {
    if (motionFlag) {
        motionFlag = false;
        return true;
    }
    return false;
}
void motion_sleep_start() {
    motionFlag = false;  // clear any pending flag
    attachInterrupt(digitalPinToInterrupt(BMI160_INT1), motionISR, RISING);
}

void motion_sleep_end() { detachInterrupt(digitalPinToInterrupt(BMI160_INT1)); }

int motion_get_steps() {
    Wire.beginTransmission(BMI160_ADDR);
    Wire.write(REG_STEP_CNT_0);
    Wire.endTransmission(false);
    Wire.requestFrom(BMI160_ADDR, 2);
    if (Wire.available() < 2) return 0;
    uint8_t low  = Wire.read();
    uint8_t high = Wire.read();
    return (int)((uint16_t)(high << 8 | low));
}