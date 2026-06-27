#include <Wire.h>
#include "config.h"
#include "globals.h"
#include "touch.h"
#include "display.h"
#include "ble_comm.h"
#include "sleep.h"

// ── Fake RTC (until module replaced) ─────────────────────────────────────
DateTime getFakeTime() {
    unsigned long seconds = millis() / 1000;
    return DateTime(2026, 1, 1,
                    (seconds / 3600) % 24,
                    (seconds / 60)   % 60,
                    seconds          % 60);
}

// ── RTC memory ────────────────────────────────────────────────────────────
RTC_DATA_ATTR int  savedScreen       = SCREEN_TIME;
RTC_DATA_ATTR char rtcNotifySrc[32]  = "";
RTC_DATA_ATTR char rtcNotifyTitle[64]= "";
RTC_DATA_ATTR char rtcNotifyBody[128]= "";
RTC_DATA_ATTR bool rtcNotifyPending  = false;

// ── Display state ─────────────────────────────────────────────────────────
unsigned long screenOnTime = 0;
bool          displayOn    = false;

void turnDisplayOn() {
    display.setBrightness(200);
    displayOn    = true;
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
    if (scrollY < 0)            scrollY = 0;
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
                expandedIndex     = i;
                currentScreen     = SCREEN_NOTIFY_EXPANDED;
                expandJustEntered = true;
                lastDraw          = 0;
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
            currentScreen = notifQueue.empty() ? prevScreen : SCREEN_NOTIFY_LIST;
            clampScroll();
            lastDraw = 0;
        }

    } else {
        currentScreen = (currentScreen == SCREEN_TIME) ? SCREEN_DATE : SCREEN_TIME;
        lastDraw = 0;
    }
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    int wakeReason = getWakeReason();
    Serial.print("Wake reason: ");
    Serial.println(wakeReason);

    resetTouch();
    pinMode(TOUCH_INT, INPUT);
    recoverI2C();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(100000);
    delay(100); // let bus settle
    //configureTouch();

    display.init();
    display.setRotation(0);
    display.fillScreen(TFT_BLACK);
    sprite.setColorDepth(8);
    sprite.createSprite(240, 240);

    if (rtcNotifyPending) {
        WatchNotification notif;
        notif.src   = String(rtcNotifySrc);
        notif.title = String(rtcNotifyTitle);
        notif.body  = String(rtcNotifyBody);
        notifQueue.push_back(notif);
        rtcNotifyPending = false;
        prevScreen    = SCREEN_TIME;
        currentScreen = SCREEN_NOTIFY_LIST;
        scrollY       = 0;
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
    static unsigned long lastTouchRead  = 0;
    static unsigned long lastPulseTime  = 0;
    static unsigned long pendingLiftTime= 0;
    static unsigned long touchCooldown  = 0;
    static bool          wasDown        = false;
    static bool          pendingLift    = false;
    static bool          isDragging     = false;
    static int           touchStartY    = 0;
    static int           touchCurrentY  = 0;

    if (now - lastTouchRead >= 20) {
        lastTouchRead = now;

        bool fingerDown = false;
        int  tx = 0, ty = 0;

        if (now > touchCooldown) {
            fingerDown = readTouch(tx, ty);
        }

        if (fingerDown && !wasDown) {
            if (now - lastPulseTime > 150) {
                // Fresh touch
                touchStartY   = ty;
                touchCurrentY = ty;
                isDragging    = false;
            }
            lastPulseTime = now;
            pendingLift   = false;
            screenOnTime  = now;
        }

        if (fingerDown && wasDown) {
            if (abs(ty - touchStartY) > TAP_THRESHOLD) {
                isDragging = true;
            }
            if (isDragging && currentScreen == SCREEN_NOTIFY_LIST) {
                int delta  = touchCurrentY - ty;
                scrollY   += delta;
                scrollVelocity = delta;
                clampScroll();
                lastDraw = 0;
            }
            touchCurrentY = ty;
            lastPulseTime = now;
            pendingLift   = false;
            screenOnTime  = now;
        }

        if (!fingerDown && wasDown) {
            pendingLiftTime = now;
            pendingLift     = true;
        }

        // Check pending lift after 80ms silence
        if (pendingLift && now - pendingLiftTime > 80) {
            pendingLift = false;
            if (now - lastPulseTime > 80) {
                // Genuine lift
                if (!isDragging) {
                    touchCooldown = now + 400;
                    processTap(touchStartY);
                }
                isDragging = false;
            }
        }

        wasDown = fingerDown;
    }

    // ── Momentum ────────────────────────────────────────────────────────────
    if (currentScreen == SCREEN_NOTIFY_LIST && abs(scrollVelocity) > 0.3f) {
        scrollY        += scrollVelocity;
        scrollVelocity *= FRICTION;
        clampScroll();
        lastDraw = 0;
    } else {
        scrollVelocity = 0;
    }

    // ── Auto return from empty list ─────────────────────────────────────────
    if (currentScreen == SCREEN_NOTIFY_LIST && notifQueue.empty()) {
        currentScreen = prevScreen;
        lastDraw      = 0;
    }

    // ── Drawing ─────────────────────────────────────────────────────────────
    bool needsRedraw = (lastDraw == 0) ||
                       (abs(scrollVelocity) > 0.3f) ||
                       (displayOn && now - lastDraw >= 1000);

    if (displayOn && needsRedraw) {
        lastDraw    = now;
        savedScreen = currentScreen;
        DateTime fakeNow = getFakeTime();

        if      (currentScreen == SCREEN_TIME)
            drawTimeScreen(fakeNow);
        else if (currentScreen == SCREEN_DATE)
            drawDateScreen(fakeNow);
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