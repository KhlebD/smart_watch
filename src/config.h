#pragma once

// Display
#define LCD_MOSI 7
#define LCD_SCLK 8
#define LCD_CS   9
#define LCD_DC   10
#define LCD_RST  11
#define LCD_BL   12

// Touch
#define TOUCH_SDA  3
#define TOUCH_SCL  4
#define TOUCH_INT  5
#define TOUCH_RST  6
#define TOUCH_ADDR 0x15

// Motion
#define BMI160_INT1 1

// Battery
#define BATTERY_PIN 2

// Heart rate on Wire1
#define HR_SDA 13
#define HR_SCL 14

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
// CHANGED TO 2000 SECONDS FOR DEBUGGING
#define DISPLAY_TIMEOUT_MS 2000000

// Touch things
#define TAP_THRESHOLD          10
#define FRICTION               0.88f
#define MAX_VELOCITY           40.0f
#define TOUCH_READ_INTERVAL_MS 20