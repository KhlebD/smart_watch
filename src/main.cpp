#include <Wire.h>

#include "NotoHebrew16.h"
#include "ble_comm.h"
#include "config.h"
#include "display.h"
#include "globals.h"
#include "sleep.h"
#include "touch.h"

// ── Screen state ─────────────────────────────────────────────────────────
int savedScreen = SCREEN_TIME;

// ── Display state ─────────────────────────────────────────────────────────
unsigned long screenOnTime = 0;
bool displayOn = false;

void turnDisplayOn() {
    display.setBrightness(200);
    displayOn = true;
    screenOnTime = millis();
}

void turnDisplayOff() {
    display.setBrightness(0);
    displayOn = false;
}

// ── Scroll helpers ────────────────────────────────────────────────────────
float maxScrollY() {
    float listH = notifQueue.size() * (CARD_HEIGHT + CARD_GAP) - CARD_GAP;
    return max(0.0f, listH - (240.0f - 28.0f));
}

void clampScroll() {
    if (scrollY < 0) scrollY = 0;
    if (scrollY > maxScrollY()) scrollY = maxScrollY();
}

// ── Tap handler ───────────────────────────────────────────────────────────
static bool expandJustEntered = false;

void processTap(int tapY) {
    if (!displayOn) {
        turnDisplayOn();
        lastDraw = 0;
        return;
    }

    if (currentScreen == SCREEN_NOTIFY_LIST) {
        int total = notifQueue.size();
        for (int i = 0; i < total; i++) {
            int cardY = 28 + i * (CARD_HEIGHT + CARD_GAP) - (int)scrollY;
            if (tapY >= cardY && tapY < cardY + CARD_HEIGHT) {
                expandedIndex = i;
                currentScreen = SCREEN_NOTIFY_EXPANDED;
                expandJustEntered = true;
                lastDraw = 0;
                break;
            }
        }

    } else if (currentScreen == SCREEN_NOTIFY_EXPANDED) {
        if (expandJustEntered) {
            expandJustEntered = false;
        } else {
            notifQueue.erase(notifQueue.begin() + expandedIndex);
            Serial.print("Dismissed, remaining: ");
            Serial.println(notifQueue.size());
            currentScreen =
                notifQueue.empty() ? prevScreen : SCREEN_NOTIFY_LIST;
            clampScroll();
            lastDraw = 0;
        }

    } else {
        currentScreen =
            (currentScreen == SCREEN_TIME) ? SCREEN_DATE : SCREEN_TIME;
        lastDraw = 0;
    }
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.printf("Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
    delay(500);

    if (!rtc.begin()) {
        Serial.println("RTC not found");
        while (1);
    }
    // rtc.adjust(DateTime(2026, 6, 28, 15, 10, 0));

    resetTouch();
    pinMode(TOUCH_INT, INPUT);
    recoverI2C();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(100000);
    delay(100);  // let bus settle
    configureTouch();
    setupTouchInterrupt(TOUCH_INT);

    display.init();
    display.setRotation(0);
    display.invertDisplay(true);
    display.fillScreen(TFT_BLACK);
    sprite.setColorDepth(16);
    sprite.setPsram(true);
    bool ok = sprite.createSprite(240, 240);
    if (!ok) {
        Serial.println("Sprite failed even with PSRAM");
        sprite.setPsram(false);
        sprite.setColorDepth(8);
        sprite.createSprite(240, 240);
    }
    Serial.printf("Hebrew font first char: 0x%04X\n", NotoHebrew16pt.first);
    Serial.printf("Hebrew font last char:  0x%04X\n", NotoHebrew16pt.last);
    turnDisplayOn();
    Serial.print("Color depth: ");
    Serial.println(sprite.getColorDepth());
    initBLE();
    Serial.println("Setup complete");
    // Temporary Hebrew font test — remove after testing
    sprite.fillScreen(TFT_BLACK);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);

    // Test ASCII with default font
    sprite.setFont(nullptr);
    sprite.setCursor(100, 100);
    sprite.print("Hello");

    /* Test Hebrew letters with Hebrew font
    sprite.setFont(&NotoHebrew16pt);
    sprite.setCursor(100, 150);
    // Draw שלום (shalom) character by character
    uint16_t shalom[] = {0x05E9, 0x05DC, 0x05D5, 0x05DD};  // ש ל ו ם
    for (int i = 0; i < 4; i++) {
        sprite.write(shalom[i]);
    }
    */
    // Test with a built-in LovyanGFX font that has extended chars

    /*
    sprite.fillScreen(TFT_BLACK);
    sprite.setFont(&fonts::FreeSans9pt7b);
    sprite.setTextColor(TFT_WHITE);
    sprite.setCursor(100, 30);
    sprite.printf("Hello");
    sprite.setCursor(100, 60);
    // Try drawing a character by codepoint directly
    sprite.drawChar(0x0041, 100, 90);   // capital A
    sprite.drawChar(0x05D0, 100, 120);  // alef - will this work?
    // Which of these compiles without error?
    sprite.drawChar(0x0042, 10, 90);        // codepoint, x, y
    sprite.drawChar(10, 90, 0x0043);        // x, y, codepoint
    sprite.drawChar(10, 90, (char)0xD0);    // just the low byte
    sprite.pushSprite(0, 0);
    delay(10000);
    */
    sprite.setFont(&NotoHebrew16pt);
    sprite.setTextColor(TFT_WHITE);
    sprite.setCursor(100, 60);
    // שלום as raw UTF-8 bytes
    sprite.drawString("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D", 100, 60);
    sprite.pushSprite(0, 0);
    delay(5000);
}

// ── Loop ──────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── Touch ──────────────────────────────────────────────────────────────
    static unsigned long lastTouchRead = 0;
    static unsigned long lastPulseTime = 0;
    static unsigned long pendingLiftTime = 0;
    static unsigned long touchCooldown = 0;
    static bool wasDown = false;
    static bool pendingLift = false;
    static bool isDragging = false;
    static int touchStartY = 0;
    static int touchCurrentY = 0;

    if (touchEventFlag) {
        touchEventFlag = false;

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

        // Touch state machine
        if (fingerDown && !wasDown) {
            touchStartY = ty;
            touchCurrentY = ty;
            isDragging = false;
            screenOnTime = now;
        }
        if (fingerDown && wasDown) {
            if (abs(ty - touchStartY) > TAP_THRESHOLD) isDragging = true;
            if (isDragging && currentScreen == SCREEN_NOTIFY_LIST) {
                int delta = touchCurrentY - ty;
                scrollY += delta;
                scrollVelocity = delta;
                clampScroll();
                lastDraw = 0;
            }
            touchCurrentY = ty;
            screenOnTime = now;
        }
        if (!fingerDown && wasDown) {
            if (!isDragging) {
                processTap(touchStartY);
            }
            isDragging = false;
        }
        wasDown = fingerDown;
    }

    // ── Momentum ────────────────────────────────────────────────────────────
    if (currentScreen == SCREEN_NOTIFY_LIST && abs(scrollVelocity) > 0.3f) {
        scrollY += scrollVelocity;
        scrollVelocity *= FRICTION;
        clampScroll();
        lastDraw = 0;
    } else {
        scrollVelocity = 0;
    }

    // ── Auto return from empty list ─────────────────────────────────────────
    if (currentScreen == SCREEN_NOTIFY_LIST && notifQueue.empty()) {
        currentScreen = prevScreen;
        lastDraw = 0;
    }

    // ── Drawing ─────────────────────────────────────────────────────────────
    bool needsRedraw = (lastDraw == 0) || (abs(scrollVelocity) > 0.3f) ||
                       (displayOn && now - lastDraw >= 1000);

    if (displayOn && needsRedraw) {
        lastDraw = now;
        savedScreen = currentScreen;
        DateTime now = rtc.now();

        if (currentScreen == SCREEN_TIME)
            drawTimeScreen(now);
        else if (currentScreen == SCREEN_DATE)
            drawDateScreen(now);
        else if (currentScreen == SCREEN_NOTIFY_LIST)
            drawNotifyListScreen();
        else if (currentScreen == SCREEN_NOTIFY_EXPANDED)
            drawNotifyExpandedScreen();
    }

    // ── Display timeout ─────────────────────────────────────────────────────
    if (displayOn && now - screenOnTime > DISPLAY_TIMEOUT_MS) {
        turnDisplayOff();
        goToLightSleep();
        turnDisplayOn();
        lastDraw = 0;
    }
}