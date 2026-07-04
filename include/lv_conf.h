#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16

// Memory — use PSRAM
#define LV_MEM_SIZE (64 * 1024U)

// Tick
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE <Arduino.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

// Font
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_DEFAULT &lv_font_montserrat_16

// BiDi for Hebrew
#define LV_USE_BIDI 1
#define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO

// Widgets
#define LV_USE_LABEL  1
#define LV_USE_BTN    1
#define LV_USE_LIST   1
#define LV_USE_ARC    1
#define LV_USE_IMG    1
#define LV_USE_LINE   1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1

// Animation
#define LV_USE_ANIM    1
#define LV_USE_SHADOW  0

// Logging
#define LV_USE_LOG 0

#endif