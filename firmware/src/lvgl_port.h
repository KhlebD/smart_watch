#pragma once
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <globals.h>

#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE <esp_heap_caps.h>
#define LV_MEM_CUSTOM_ALLOC(size) heap_caps_malloc(size, MALLOC_CAP_SPIRAM)
#define LV_MEM_CUSTOM_FREE  heap_caps_free

void lvgl_port_init(LGFX &display);