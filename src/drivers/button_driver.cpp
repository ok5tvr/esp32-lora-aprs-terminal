#include "drivers/button_driver.h"

#include <Arduino.h>

#include "app_config.h"
#include "app_log.h"
#include "board_pins.h"

namespace Drivers {

bool ButtonDriver::begin() {
    pinMode(BoardPins::BOOT_BUTTON, INPUT_PULLUP);

    rawPressed_ = digitalRead(BoardPins::BOOT_BUTTON) == LOW;
    stablePressed_ = rawPressed_;
    ignoreUntilReleased_ = rawPressed_;
    rawChangedAtMs_ = millis();
    pressedAtMs_ = rawPressed_ ? rawChangedAtMs_ : 0;
    lastClickAtMs_ = 0;
    bootClickPending_ = false;
    initialized_ = true;

    LOG_I(
        "BUTTON",
        "BOOT button ready on GPIO%d, active LOW",
        BoardPins::BOOT_BUTTON);
    return true;
}

void ButtonDriver::update(std::uint32_t now) {
    if (!initialized_) {
        return;
    }

    const bool pressed = digitalRead(BoardPins::BOOT_BUTTON) == LOW;
    if (pressed != rawPressed_) {
        rawPressed_ = pressed;
        rawChangedAtMs_ = now;
    }

    if (rawPressed_ == stablePressed_ ||
        now - rawChangedAtMs_ < AppConfig::BUTTON_DEBOUNCE_MS) {
        return;
    }

    stablePressed_ = rawPressed_;
    if (stablePressed_) {
        pressedAtMs_ = now;
        return;
    }

    if (ignoreUntilReleased_) {
        ignoreUntilReleased_ = false;
        return;
    }

    const std::uint32_t pressDuration = now - pressedAtMs_;
    if (pressDuration < AppConfig::BUTTON_MIN_CLICK_MS ||
        pressDuration > AppConfig::BUTTON_MAX_CLICK_MS) {
        return;
    }

    if (lastClickAtMs_ != 0 &&
        now - lastClickAtMs_ < AppConfig::BUTTON_CLICK_COOLDOWN_MS) {
        return;
    }

    lastClickAtMs_ = now;
    bootClickPending_ = true;
    LOG_I(
        "BUTTON",
        "BOOT short click detected (%u ms)",
        static_cast<unsigned>(pressDuration));
}

bool ButtonDriver::consumeBootClick() {
    const bool pending = bootClickPending_;
    bootClickPending_ = false;
    return pending;
}

}  // namespace Drivers
