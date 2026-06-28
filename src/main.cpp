#include <Wire.h>

#include "ble_comm.h"
#include "config.h"
#include "display.h"
#include "globals.h"
#include "sleep.h"
#include "touch.h"

// ── RTC memory ────────────────────────────────────────────────────────────
RTC_DATA_ATTR int savedScreen = SCREEN_TIME;
RTC_DATA_ATTR char rtcNotifySrc[32] = "";
RTC_DATA_ATTR char rtcNotifyTitle[64] = "";
RTC_DATA_ATTR char rtcNotifyBody[128] = "";
RTC_DATA_ATTR bool rtcNotifyPending = false;

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
    //rtc.adjust(DateTime(2026, 6, 28, 15, 10, 0));
    int wakeReason = getWakeReason();
    Serial.print("Wake reason: ");
    Serial.println(wakeReason);

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
    Serial.print("Color depth: ");
    Serial.println(sprite.getColorDepth());

    if (rtcNotifyPending) {
        WatchNotification notif;
        notif.src = String(rtcNotifySrc);
        notif.title = String(rtcNotifyTitle);
        notif.body = String(rtcNotifyBody);
        notifQueue.push_back(notif);
        rtcNotifyPending = false;
        prevScreen = SCREEN_TIME;
        currentScreen = SCREEN_NOTIFY_LIST;
        scrollY = 0;
    } else {
        currentScreen = savedScreen;
    }

    if (wakeReason == WAKE_REASON_TIMER) {
        turnDisplayOff();
    } else {
        turnDisplayOn();
    }

    initBLE();
    configureSleepWakeSources();
    Serial.println("Setup complete");
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

    // ── Display timeout (comment out for testing) ───────────────────────────
    // if (displayOn && now - screenOnTime > DISPLAY_TIMEOUT_MS) {
    //     savedScreen = currentScreen;
    //     turnDisplayOff();
    //     goToSleep();
    // }

    // ── Timer wake timeout (comment out for testing) ────────────────────────
    // static unsigned long timerWakeTime = millis();
    // if (!displayOn && now - timerWakeTime > 5000) {
    //     goToSleep();
    // }
}