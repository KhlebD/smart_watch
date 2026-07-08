#pragma once

// Display pins
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS   5
#define LCD_DC   26
#define LCD_RST  25
#define LCD_BL   4

// Touch pins
#define TOUCH_SDA  21
#define TOUCH_SCL  22
#define TOUCH_INT  33 // changed from 19, WROVER prototyping only — RTC-capable pin needed for deep sleep wake
#define TOUCH_RST  15
#define TOUCH_ADDR 0x15

// Battery
#define BATTERY_PIN 34

// Motion
#define BMI160_INT1 32

// BLE UUIDs
#define BLE_SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_RX_UUID       "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_TX_UUID       "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Screen states
#define SCREEN_TIME    0
#define SCREEN_DATE    1
#define SCREEN_NOTIFY  2

#define SCREEN_TIME            0
#define SCREEN_DATE            1
#define SCREEN_NOTIFY          2  // keep for RTC memory compat
#define SCREEN_NOTIFY_LIST     3
#define SCREEN_NOTIFY_EXPANDED 4

#define CARDS_PER_PAGE         3
#define CARD_HEIGHT            58
#define CARD_GAP               6
#define CARD_PADDING           8

// Sleep time
#define SLEEP_DURATION_SEC 20
#define DISPLAY_TIMEOUT_MS 20000  // screen stays on 10 seconds after touch wake

// Touch things
#define TAP_THRESHOLD          10
#define FRICTION               0.88f
#define MAX_VELOCITY           40.0f
#define TOUCH_READ_INTERVAL_MS 20