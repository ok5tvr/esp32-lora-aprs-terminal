#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* app_lvgl_malloc(size_t size);
void app_lvgl_free(void* pointer);
void* app_lvgl_realloc(void* pointer, size_t size);

#ifdef __cplusplus
}
#endif
