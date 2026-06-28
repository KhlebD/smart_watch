#pragma once
#include <LovyanGFX.hpp>
#include <RTClib.h>
#include <NimBLEDevice.h>
#include <queue>
#include <vector>

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_GC9A01  _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Light_PWM     _light_instance;

public:
    LGFX(void);
};
// Queue (Vector)
struct WatchNotification {
    String src;
    String title;
    String body;
};

extern std::vector<WatchNotification> notifQueue;
extern volatile int scrollOffset;
extern volatile int expandedIndex;
// Smooth scroll state
extern float scrollY;
extern float scrollVelocity;

// Shared objects — defined in globals.cpp
extern LGFX display;
extern lgfx::LGFX_Sprite sprite;
extern RTC_DS3231 rtc;
extern BLEServer* pServer;
extern BLECharacteristic* pTxCharacteristic;
extern volatile bool pendingRedraw;
// Shared state
extern volatile bool bleConnected;
extern volatile int currentScreen;
extern volatile int prevScreen;
extern volatile unsigned long lastDraw;
extern volatile unsigned long notifyTime;

extern String notifySrc;
extern String notifyTitle;
extern String notifyBody;
extern String bleBuffer;

extern volatile bool touchEventFlag;