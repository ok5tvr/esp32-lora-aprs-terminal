#include "drivers/lvgl_port.h"

#include <Arduino.h>
#include <cstdint>
#include <esp_heap_caps.h>
#include <lvgl.h>

#include "app_config.h"
#include "app_log.h"
#include "board_pins.h"
#include "drivers/display_driver.h"
#include "drivers/touch_driver.h"

namespace Drivers {
namespace LvglPort {
namespace {

lv_color_t* buffer = nullptr;
lv_disp_draw_buf_t drawBuffer;
lv_disp_drv_t displayDriver;
lv_indev_drv_t inputDriver;
std::uint32_t lastTick = 0;

void flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* pixels) {
    const std::uint16_t width = static_cast<std::uint16_t>(area->x2 - area->x1 + 1);
    const std::uint16_t height = static_cast<std::uint16_t>(area->y2 - area->y1 + 1);
    Display::drawRgb565Bitmap(
        area->x1,
        area->y1,
        reinterpret_cast<std::uint16_t*>(&pixels->full),
        width,
        height,
        LV_COLOR_16_SWAP != 0);
    lv_disp_flush_ready(driver);
}

void readTouch(lv_indev_drv_t*, lv_indev_data_t* data) {
    std::int16_t x = 0;
    std::int16_t y = 0;
    if (!Touch::readPoint(x, y)) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
}

bool allocateDrawBuffer() {
    const std::size_t pixelCount =
        static_cast<std::size_t>(BoardPins::SCREEN_WIDTH) *
        AppConfig::LVGL_BUFFER_LINES;
    const std::size_t byteCount = pixelCount * sizeof(lv_color_t);

    // The display transport may use DMA, therefore the draw buffer stays in
    // internal DMA-capable RAM. It is allocated at runtime instead of .bss.
    buffer = static_cast<lv_color_t*>(heap_caps_malloc(
        byteCount,
        MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

    if (buffer == nullptr) {
        LOG_E(
            "LVGL",
            "Draw buffer allocation failed: %u bytes",
            static_cast<unsigned>(byteCount));
        return false;
    }

    LOG_I(
        "LVGL",
        "Draw buffer: %u lines, %u bytes internal DMA RAM",
        static_cast<unsigned>(AppConfig::LVGL_BUFFER_LINES),
        static_cast<unsigned>(byteCount));
    return true;
}

}  // namespace

bool begin() {
    if (!allocateDrawBuffer()) {
        return false;
    }

    lv_init();
    lv_disp_draw_buf_init(
        &drawBuffer,
        buffer,
        nullptr,
        BoardPins::SCREEN_WIDTH * AppConfig::LVGL_BUFFER_LINES);

    lv_disp_drv_init(&displayDriver);
    displayDriver.hor_res = BoardPins::SCREEN_WIDTH;
    displayDriver.ver_res = BoardPins::SCREEN_HEIGHT;
    displayDriver.flush_cb = flush;
    displayDriver.draw_buf = &drawBuffer;
    lv_disp_drv_register(&displayDriver);

    lv_indev_drv_init(&inputDriver);
    inputDriver.type = LV_INDEV_TYPE_POINTER;
    inputDriver.read_cb = readTouch;
    lv_indev_drv_register(&inputDriver);

    lastTick = millis();

    LOG_I(
        "LVGL",
        "Allocator ready; internal free %u bytes, PSRAM free %u bytes",
        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
    return true;
}

void update() {
    const std::uint32_t now = millis();
    const std::uint32_t elapsed = now - lastTick;
    if (elapsed > 0) {
        lv_tick_inc(elapsed);
        lastTick = now;
    }
    lv_timer_handler();
}

}  // namespace LvglPort
}  // namespace Drivers
