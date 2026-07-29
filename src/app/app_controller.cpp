#include "app/app_controller.h"

#include <Arduino.h>
#include <cmath>
#include <cstdio>
#include <esp_heap_caps.h>
#include <esp_system.h>

#include "app_config.h"
#include "app_log.h"
#include "drivers/display_driver.h"
#include "drivers/lvgl_port.h"
#include "drivers/sd_card_driver.h"
#include "drivers/touch_driver.h"

namespace App {
namespace {

const char* resetReasonName(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON: return "power-on";
        case ESP_RST_EXT: return "external reset";
        case ESP_RST_SW: return "software reset";
        case ESP_RST_PANIC: return "panic/exception";
        case ESP_RST_INT_WDT: return "interrupt watchdog";
        case ESP_RST_TASK_WDT: return "task watchdog";
        case ESP_RST_WDT: return "other watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "SDIO reset";
        default: return "unknown";
    }
}

void copyError(
    char* output,
    std::size_t outputCapacity,
    const char* text) {

    if (output == nullptr || outputCapacity == 0) {
        return;
    }
    std::snprintf(output, outputCapacity, "%s", text != nullptr ? text : "");
}

}  // namespace

bool AppController::begin() {
    Serial.begin(AppConfig::SERIAL_BAUD_RATE);
    delay(100);
    Serial.println();
    LOG_I("APP", "%s v%s", AppConfig::FIRMWARE_NAME, AppConfig::FIRMWARE_VERSION);
    const esp_reset_reason_t resetReason = esp_reset_reason();
    LOG_I("APP", "Previous reset: %s (%d)", resetReasonName(resetReason), static_cast<int>(resetReason));
    LOG_I("APP", "Board: %s", AppConfig::BOARD_NAME);
    LOG_I(
        "APP",
        "Flash %u MB, PSRAM %u MB%s",
        static_cast<unsigned>(ESP.getFlashChipSize() / (1024U * 1024U)),
        static_cast<unsigned>(ESP.getPsramSize() / (1024U * 1024U)),
        psramFound() ? "" : " (not detected)");

    if (!Drivers::Display::begin()) {
        halt("Display initialization failed");
    }
    if (!Drivers::Touch::begin()) {
        halt("Touch initialization failed");
    }
    if (!Drivers::LvglPort::begin()) {
        halt("LVGL initialization failed");
    }

    if (AppConfig::ENABLE_SD_CARD && !Drivers::SdCard::begin()) {
        LOG_E("APP", "SD card is unavailable; UI and LoRa remain active");
    }

    if (!settings_.begin()) {
        LOG_E("APP", "Persistent settings unavailable; defaults remain active");
    }

    if (!buttons_.begin()) {
        LOG_E("APP", "Onboard BOOT button is unavailable");
    }

    if (AppConfig::ENABLE_GPS) {
        gps_.begin();
    }

    // Arduino_GFX/VSPI is initialized before the independent LoRa HSPI bus.
    if (AppConfig::ENABLE_LORA && !radio_.begin()) {
        LOG_E("APP", "LoRa is unavailable; UI remains active");
    }

    tracker_.begin();
    updateReferencePosition();
    screens_.begin(
        commandThunk,
        this,
        messageSendThunk,
        this,
        settingsSaveThunk,
        this,
        digiIgateSettingsSaveThunk,
        this,
        trackerSettingsSaveThunk,
        this);
    LOG_I(
        "APP",
        "Startup memory: internal %u bytes, PSRAM %u bytes",
        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
    return true;
}

void AppController::update() {
    const std::uint32_t now = millis();
    Drivers::LvglPort::update();
    buttons_.update(now);
    const bool bootBeaconRequested = buttons_.consumeBootClick();
    if (bootBeaconRequested) {
        tracker_.requestImmediateBeacon(now);
    }

    if (AppConfig::ENABLE_GPS) {
        gps_.update(now);
    }
    if (AppConfig::ENABLE_LORA) {
        radio_.update(now, settings_.viewState());
    }
    tracker_.update(now, settings_.viewState(), gps_.viewState(), radio_);
    updateReferencePosition();

    screens_.update(
        now,
        radio_.viewState(),
        radio_.messageViewState(),
        radio_.stationViewState(),
        radio_.weatherViewState(),
        gps_.viewState(),
        tracker_.viewState(),
        radio_.digiIgateViewState(),
        referencePosition_,
        settings_.viewState());

    const Services::TrackerService::ViewState& trackerState = tracker_.viewState();
    if (trackerState.manualPacketsSent != observedManualPacketsSent_) {
        observedManualPacketsSent_ = trackerState.manualPacketsSent;
        screens_.setMessage("BOOT: pozicni beacon byl odeslan.");
    } else if (trackerState.manualBeaconFailures != observedManualBeaconFailures_) {
        observedManualBeaconFailures_ = trackerState.manualBeaconFailures;
        screens_.setMessage(trackerState.statusText);
    } else if (bootBeaconRequested) {
        screens_.setMessage(trackerState.statusText);
    }

    delay(AppConfig::MAIN_LOOP_DELAY_MS);
}

void AppController::commandThunk(Command command, void* context) {
    if (context != nullptr) {
        static_cast<AppController*>(context)->handleCommand(command);
    }
}

bool AppController::messageSendThunk(
    const char* recipient,
    const char* text,
    char* errorText,
    std::size_t errorTextCapacity,
    void* context) {

    if (context == nullptr) {
        return false;
    }
    return static_cast<AppController*>(context)->sendMessage(
        recipient,
        text,
        errorText,
        errorTextCapacity);
}

bool AppController::settingsSaveThunk(
    const char* callsign,
    double latitude,
    double longitude,
    char* errorText,
    std::size_t errorTextCapacity,
    void* context) {

    if (context == nullptr) {
        return false;
    }
    return static_cast<AppController*>(context)->saveSettings(
        callsign,
        latitude,
        longitude,
        errorText,
        errorTextCapacity);
}

bool AppController::digiIgateSettingsSaveThunk(
    bool digiEnabled,
    DigiMode digiMode,
    std::uint8_t maxWideHops,
    bool igateEnabled,
    const char* wifiSsid,
    const char* wifiPassword,
    const char* aprsIsServer,
    std::uint16_t aprsIsPort,
    std::int32_t aprsIsPasscode,
    const char* aprsIsFilter,
    char* errorText,
    std::size_t errorTextCapacity,
    void* context) {

    if (context == nullptr) {
        return false;
    }
    return static_cast<AppController*>(context)->saveDigiIgateSettings(
        digiEnabled,
        digiMode,
        maxWideHops,
        igateEnabled,
        wifiSsid,
        wifiPassword,
        aprsIsServer,
        aprsIsPort,
        aprsIsPasscode,
        aprsIsFilter,
        errorText,
        errorTextCapacity);
}

bool AppController::trackerSettingsSaveThunk(
    bool enabled,
    TrackerPositionSource source,
    TrackerPositionFormat format,
    TrackerBeaconMode mode,
    TrackerSymbol symbol,
    std::uint32_t fixedIntervalSeconds,
    char* errorText,
    std::size_t errorTextCapacity,
    void* context) {

    if (context == nullptr) {
        return false;
    }
    return static_cast<AppController*>(context)->saveTrackerSettings(
        enabled,
        source,
        format,
        mode,
        symbol,
        fixedIntervalSeconds,
        errorText,
        errorTextCapacity);
}

bool AppController::saveSettings(
    const char* callsign,
    double latitude,
    double longitude,
    char* errorText,
    std::size_t errorTextCapacity) {

    return settings_.save(callsign, latitude, longitude, errorText, errorTextCapacity);
}

bool AppController::saveDigiIgateSettings(
    bool digiEnabled,
    DigiMode digiMode,
    std::uint8_t maxWideHops,
    bool igateEnabled,
    const char* wifiSsid,
    const char* wifiPassword,
    const char* aprsIsServer,
    std::uint16_t aprsIsPort,
    std::int32_t aprsIsPasscode,
    const char* aprsIsFilter,
    char* errorText,
    std::size_t errorTextCapacity) {

    if ((digiEnabled || igateEnabled) && !radio_.viewState().initialized) {
        copyError(
            errorText,
            errorTextCapacity,
            "DIGI/iGate nelze zapnout: radio neni inicializovano.");
        return false;
    }

    return settings_.saveDigiIgate(
        digiEnabled,
        digiMode,
        maxWideHops,
        igateEnabled,
        wifiSsid,
        wifiPassword,
        aprsIsServer,
        aprsIsPort,
        aprsIsPasscode,
        aprsIsFilter,
        errorText,
        errorTextCapacity);
}

bool AppController::saveTrackerSettings(
    bool enabled,
    TrackerPositionSource source,
    TrackerPositionFormat format,
    TrackerBeaconMode mode,
    TrackerSymbol symbol,
    std::uint32_t fixedIntervalSeconds,
    char* errorText,
    std::size_t errorTextCapacity) {

    if (enabled && !radio_.viewState().initialized) {
        copyError(errorText, errorTextCapacity, "Tracker nelze zapnout: radio neni inicializovano.");
        return false;
    }
    if (enabled && source == TrackerPositionSource::Gps &&
        !gps_.viewState().receiverDetected) {
        copyError(errorText, errorTextCapacity, "Tracker nelze zapnout: GPS nebyla nalezena.");
        return false;
    }

    return settings_.saveTracker(
        enabled,
        source,
        format,
        mode,
        symbol,
        fixedIntervalSeconds,
        errorText,
        errorTextCapacity);
}

bool AppController::sendMessage(
    const char* recipient,
    const char* text,
    char* errorText,
    std::size_t errorTextCapacity) {

    if (!radio_.viewState().initialized) {
        copyError(errorText, errorTextCapacity, "Radio neni inicializovano.");
        return false;
    }
    return radio_.queueMessage(
        recipient,
        text,
        millis(),
        errorText,
        errorTextCapacity);
}

void AppController::handleCommand(Command command) {
    if (command == Command::SendTestPacket) {
        if (!radio_.viewState().initialized) {
            screens_.setMessage("Radio neni inicializovano.");
            return;
        }
        if (radio_.viewState().transmitting) {
            screens_.setMessage("Predchozi paket se jeste vysila.");
            return;
        }
        const bool started = radio_.sendTestPacket(settings_.viewState().callsign, millis());
        screens_.setMessage(started ? "Testovaci APRS paket byl zarazen k vysilani."
                                    : "Vysilani se nepodarilo spustit.");
    }
}

void AppController::updateReferencePosition() {
    bool candidateValid = true;
    bool candidateFromGps = false;
    double candidateLatitude = settings_.viewState().defaultLatitude;
    double candidateLongitude = settings_.viewState().defaultLongitude;

    if (gps_.viewState().hasFix) {
        candidateFromGps = true;
        candidateLatitude = gps_.viewState().latitude;
        candidateLongitude = gps_.viewState().longitude;
    }

    bool changed = referencePosition_.valid != candidateValid ||
        referencePosition_.fromGps != candidateFromGps;
    if (!changed && referencePosition_.valid) {
        const Services::DistanceBearing movement = Services::calculateDistanceBearing(
            referencePosition_.latitude,
            referencePosition_.longitude,
            candidateLatitude,
            candidateLongitude);
        changed = !movement.valid || movement.distanceKm >= 0.01;
    }

    if (changed) {
        referencePosition_.valid = candidateValid;
        referencePosition_.fromGps = candidateFromGps;
        referencePosition_.latitude = candidateLatitude;
        referencePosition_.longitude = candidateLongitude;
        ++referencePosition_.revision;
    }
}

[[noreturn]] void AppController::halt(const char* reason) {
    LOG_E("APP", "%s", reason);
    while (true) {
        delay(1000);
    }
}

}  // namespace App
