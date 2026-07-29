#include "drivers/touch_driver.h"

#include <Arduino.h>
#include <TouchDrvFT6X36.hpp>
#include <Wire.h>

#include "app_log.h"
#include "board_pins.h"

namespace Drivers {
namespace Touch {
namespace {

TouchDrvFT6X36 controller;

void transform(std::int16_t rawX, std::int16_t rawY, std::int16_t& x, std::int16_t& y) {
    std::int32_t mappedX = rawX;
    std::int32_t mappedY = rawY;

    if (BoardPins::TOUCH_SWAP_XY) {
        const std::int32_t temporary = mappedX;
        mappedX = mappedY;
        mappedY = temporary;
    }
    if (BoardPins::TOUCH_MIRROR_X) {
        mappedX = static_cast<std::int32_t>(BoardPins::SCREEN_WIDTH) - 1 - mappedX;
    }
    if (BoardPins::TOUCH_MIRROR_Y) {
        mappedY = static_cast<std::int32_t>(BoardPins::SCREEN_HEIGHT) - 1 - mappedY;
    }

    mappedX = constrain(mappedX, 0, static_cast<std::int32_t>(BoardPins::SCREEN_WIDTH) - 1);
    mappedY = constrain(mappedY, 0, static_cast<std::int32_t>(BoardPins::SCREEN_HEIGHT) - 1);
    x = static_cast<std::int16_t>(mappedX);
    y = static_cast<std::int16_t>(mappedY);
}

}  // namespace

bool begin() {
    if (!controller.begin(Wire, BoardPins::TOUCH_ADDRESS)) {
        LOG_E("TOUCH", "FT6336 not found");
        return false;
    }
    LOG_I("TOUCH", "Ready");
    return true;
}

bool readPoint(std::int16_t& x, std::int16_t& y) {
    std::int16_t rawX[1] = {0};
    std::int16_t rawY[1] = {0};
    if (controller.getPoint(rawX, rawY, 1) == 0) {
        return false;
    }
    transform(rawX[0], rawY[0], x, y);
    return true;
}

}  // namespace Touch
}  // namespace Drivers
