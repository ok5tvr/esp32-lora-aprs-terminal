#include "services/display_power_service.h"

#include <Arduino.h>

#include <algorithm>

#include "app_config.h"
#include "app_log.h"
#include "drivers/display_driver.h"
#include "drivers/lvgl_port.h"

namespace Services {

bool DisplayPowerService::begin(
    std::uint32_t now,
    const SettingsService::ViewState& settings,
    const PowerService::ViewState& power) {

    view_ = ViewState{};
    view_.batteryMode = isBatteryMode(power);
    lastActivityAt_ = now;
    initialized_ = true;
    Drivers::LvglPort::setTouchWakeOnly(false);
    setFull(settings, true);
    LOG_I(
        "DISPLAY_PWR",
        "Ready: source=%s brightness=%u%% dim=%u%% after=%u s off=%u s",
        view_.batteryMode ? "battery" : "USB/full",
        static_cast<unsigned>(view_.appliedBrightnessPercent),
        static_cast<unsigned>(dimBrightness(settings)),
        static_cast<unsigned>(AppConfig::DISPLAY_DIM_AFTER_SECONDS),
        static_cast<unsigned>(settings.displayTimeoutSeconds));
    return true;
}

bool DisplayPowerService::update(
    std::uint32_t now,
    const SettingsService::ViewState& settings,
    const PowerService::ViewState& power,
    bool userActivity) {

    if (!initialized_) {
        begin(now, settings, power);
    }

    bool wokeFromActivity = false;
    const bool batteryMode = isBatteryMode(power);
    if (batteryMode != view_.batteryMode) {
        view_.batteryMode = batteryMode;
        lastActivityAt_ = now;
        if (!batteryMode) {
            wake(now, settings);
            LOG_I("DISPLAY_PWR", "USB-C detected: full brightness, no dimming or blanking");
        } else {
            Drivers::LvglPort::setTouchWakeOnly(false);
            setFull(settings, true);
            LOG_I(
                "DISPLAY_PWR",
                "Battery mode: brightness %u%%, dim after %u s",
                static_cast<unsigned>(settings.batteryBrightnessPercent),
                static_cast<unsigned>(AppConfig::DISPLAY_DIM_AFTER_SECONDS));
        }
        ++view_.revision;
    }

    if (!view_.batteryMode) {
        if (view_.displayOff || view_.displayDimmed) {
            wake(now, settings);
        }
        setFull(settings);
        return false;
    }

    if (userActivity) {
        lastActivityAt_ = now;
        if (view_.displayOff) {
            wokeFromActivity = wake(now, settings);
        } else if (view_.displayDimmed) {
            setFull(settings, true);
            LOG_I("DISPLAY_PWR", "Display restored from dimmed state");
        }
    }

    if (view_.displayOff) {
        return wokeFromActivity;
    }

    const std::uint32_t inactiveMs = now - lastActivityAt_;
    const std::uint32_t dimAfterMs =
        static_cast<std::uint32_t>(AppConfig::DISPLAY_DIM_AFTER_SECONDS) * 1000U;
    const std::uint16_t timeoutSeconds = settings.displayTimeoutSeconds;
    const std::uint32_t timeoutMs = static_cast<std::uint32_t>(timeoutSeconds) * 1000U;

    // The configured off timeout remains authoritative. With the default 60 s
    // this produces exactly: 0-30 s normal, 30-60 s 15 %, then off.
    if (timeoutSeconds > 0U && inactiveMs >= timeoutMs) {
        blank();
    } else if (inactiveMs >= dimAfterMs) {
        setDimmed(settings);
    } else {
        setFull(settings);
    }

    return wokeFromActivity;
}

const DisplayPowerService::ViewState& DisplayPowerService::viewState() const {
    return view_;
}

bool DisplayPowerService::isBatteryMode(const PowerService::ViewState& power) {
    // Fail safe: if the PMIC is unavailable, leave the display fully active.
    return power.available && power.batteryConnected &&
        !power.vbusConnected && !power.vbusGood;
}

std::uint8_t DisplayPowerService::fullBrightness(
    const SettingsService::ViewState& settings) const {

    return view_.batteryMode
        ? settings.batteryBrightnessPercent
        : AppConfig::DISPLAY_USB_BRIGHTNESS_PERCENT;
}

std::uint8_t DisplayPowerService::dimBrightness(
    const SettingsService::ViewState& settings) const {

    return std::min(
        settings.batteryBrightnessPercent,
        AppConfig::DISPLAY_DIM_BRIGHTNESS_PERCENT);
}

void DisplayPowerService::applyBrightness(std::uint8_t percent, bool force) {
    if (!force && percent == lastRequestedBrightnessPercent_) {
        return;
    }
    lastRequestedBrightnessPercent_ = percent;
    const std::uint8_t previous = view_.appliedBrightnessPercent;
    Drivers::Display::setBacklightPercent(percent);
    view_.appliedBrightnessPercent = Drivers::Display::backlightPercent();
    if (view_.appliedBrightnessPercent != previous) {
        ++view_.revision;
    }
}

bool DisplayPowerService::wake(
    std::uint32_t now,
    const SettingsService::ViewState& settings) {

    const bool wasOff = view_.displayOff;
    lastActivityAt_ = now;
    Drivers::LvglPort::setTouchWakeOnly(false);
    setFull(settings, true);
    if (wasOff) {
        LOG_I("DISPLAY_PWR", "Display woke");
    }
    return wasOff;
}

void DisplayPowerService::setFull(
    const SettingsService::ViewState& settings,
    bool force) {

    const Stage previous = view_.stage;
    view_.stage = Stage::Full;
    view_.displayOff = false;
    view_.displayDimmed = false;
    applyBrightness(fullBrightness(settings), force);
    if (previous != view_.stage) {
        ++view_.revision;
    }
}

void DisplayPowerService::setDimmed(
    const SettingsService::ViewState& settings) {

    if (view_.stage == Stage::Dimmed) {
        applyBrightness(dimBrightness(settings));
        return;
    }
    view_.stage = Stage::Dimmed;
    view_.displayOff = false;
    view_.displayDimmed = true;
    applyBrightness(dimBrightness(settings), true);
    ++view_.revision;
    LOG_I(
        "DISPLAY_PWR",
        "Display dimmed to %u%%",
        static_cast<unsigned>(view_.appliedBrightnessPercent));
}

void DisplayPowerService::blank() {
    if (view_.displayOff) {
        return;
    }
    view_.stage = Stage::Off;
    view_.displayOff = true;
    view_.displayDimmed = false;
    applyBrightness(0, true);
    Drivers::LvglPort::setTouchWakeOnly(true);
    ++view_.revision;
    LOG_I("DISPLAY_PWR", "Display blanked after inactivity");
}

}  // namespace Services
