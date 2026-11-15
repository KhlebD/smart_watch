#include "globals.h"
#include "config.h"
#include <lvgl.h>

LGFX::LGFX(void) {
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI2_HOST;
        cfg.freq_write = 80000000;
        cfg.pin_mosi   = LCD_MOSI;
        cfg.pin_miso   = -1;
        
        cfg.pin_sclk   = LCD_SCLK;
        cfg.pin_dc     = LCD_DC;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
    }
    {
        auto cfg = _panel_instance.config();
        cfg.pin_cs   = LCD_CS;
        cfg.pin_rst  = LCD_RST;
        cfg.pin_busy = -1;
        _panel_instance.config(cfg);
    }
    {
        auto cfg = _light_instance.config();
        cfg.pin_bl = LCD_BL;
        cfg.invert = false;
        _light_instance.config(cfg);
        _panel_instance.setLight(&_light_instance);
    }
    setPanel(&_panel_instance);
}

// Object definitions
LGFX display;
lgfx::LGFX_Sprite sprite(&display);
RTC_DS3231 rtc;
BLEServer* pServer = nullptr;
BLECharacteristic* pTxCharacteristic = nullptr;
std::vector<WatchNotification> notifQueue;
volatile int scrollOffset  = 0;
volatile int expandedIndex = 0;
float scrollY       = 0.0f;
float scrollVelocity = 0.0f;
// State definitions
volatile bool bleConnected = false;
volatile int currentScreen = SCREEN_TIME;
volatile int prevScreen    = SCREEN_TIME;
volatile unsigned long lastDraw   = 0;
volatile unsigned long notifyTime = 0;
volatile bool pendingRedraw = false;

volatile bool touchEventFlag = false;

String bleBuffer   = "";

int  lastTouchX    = 0;
int  lastTouchY    = 0;
bool lastTouchDown = false;

lv_obj_t *scr_watch = nullptr;
volatile bool pendingNotification = false;

int g_hour   = 0;
int g_minute = 0;

// Spotify
String music_track           = "Not playing";
String music_artist          = "";
String music_state           = "paused";
int    music_pos             = 0;
int    music_dur             = 0;
int    music_vol             = 70;
unsigned long music_pos_updated_at = 0;

// Running
String run_state     = "idle";
float  run_dist_km   = 0.0f;
long   run_time_sec  = 0;
float  run_pace_min_km = 0.0f;
float  run_speed_kmh = 0.0f;
bool   run_active    = false;

