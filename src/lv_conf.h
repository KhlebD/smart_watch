#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        0
#define LV_COLOR_SCREEN_TRANSP  0

#define LV_MEM_CUSTOM           0
#define LV_MEM_SIZE             (48 * 1024U)

#define LV_TICK_CUSTOM          1
#define LV_TICK_CUSTOM_INCLUDE  <Arduino.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_DPI_DEF              130

// Fonts
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_DEFAULT         &lv_font_montserrat_16

// BiDi Hebrew support
#define LV_USE_BIDI             1
#define LV_BIDI_BASE_DIR_DEF    LV_BASE_DIR_AUTO

// Text
#define LV_TXT_ENC              LV_TXT_ENC_UTF8

// Widgets
#define LV_USE_ARC              1
#define LV_USE_BAR              1
#define LV_USE_BTN              1
#define LV_USE_CANVAS           0
#define LV_USE_CHECKBOX         0
#define LV_USE_DROPDOWN         1
#define LV_USE_IMG              1
#define LV_USE_LABEL            1
#define LV_USE_LINE             1
#define LV_USE_ROLLER           1
#define LV_USE_SLIDER           1
#define LV_USE_SWITCH           1
#define LV_USE_TEXTAREA         1
#define LV_USE_TABLE            0
#define LV_USE_LIST             1
#define LV_USE_TABVIEW          1
#define LV_USE_TILEVIEW         1
#define LV_USE_SPINNER          1

// Theme
#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   1
#define LV_USE_THEME_SIMPLE     1
#define LV_USE_THEME_MONO       0

// Layouts
#define LV_USE_FLEX             1
#define LV_USE_GRID             1

// Logging
#define LV_USE_LOG              0

// Asserts
#define LV_USE_ASSERT_NULL      1
#define LV_USE_ASSERT_MALLOC    1
#define LV_USE_ASSERT_STYLE     0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ       0

// Animation
#define LV_USE_ANIMATION        1
#define LV_USE_SHADOW           1
#define LV_USE_BLEND_MODES      1
#define LV_USE_OPA_SCALE        1
#define LV_USE_IMG_TRANSFORM    1
#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0
#define LV_USE_REFR_DEBUG       0

#define LV_USE_GPU_NXP_VG_LITE  0
#define LV_USE_GPU_NXP_PXP      0
#define LV_USE_GPU_STM32_DMA2D  0
#define LV_USE_GPU_SDL          0

#define LV_USE_PNG              0
#define LV_USE_BMP              0
#define LV_USE_JPG              0
#define LV_USE_GIF              0
#define LV_USE_QRCODE           0
#define LV_USE_FREETYPE         0
#define LV_USE_RLOTTIE          0
#define LV_USE_FFMPEG           0

#endif