#include "lvgl_memory.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

extern "C" void* app_lvgl_malloc(size_t size) {
    if (size == 0) {
        return nullptr;
    }

    void* pointer = nullptr;
    if (psramFound()) {
        pointer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (pointer == nullptr) {
        pointer = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    return pointer;
}

extern "C" void app_lvgl_free(void* pointer) {
    if (pointer != nullptr) {
        heap_caps_free(pointer);
    }
}

extern "C" void* app_lvgl_realloc(void* pointer, size_t size) {
    if (pointer == nullptr) {
        return app_lvgl_malloc(size);
    }
    if (size == 0) {
        app_lvgl_free(pointer);
        return nullptr;
    }

    void* resized = nullptr;
    if (psramFound()) {
        resized = heap_caps_realloc(
            pointer,
            size,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (resized == nullptr) {
        resized = heap_caps_realloc(
            pointer,
            size,
            MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    return resized;
}
