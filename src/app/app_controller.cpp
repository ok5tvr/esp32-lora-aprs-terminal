#include "app/app_controller.h"

#include <Arduino.h>
#include <cmath>
#include <cstdio>
#include <esp_heap_caps.h>
#include <esp_system.h>

#include "app_config.h"
#include "app/localization.h"
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

    // Load the persistent language before hardware services create their
    // first user-visible status strings.
    if (!settings_.begin()) {
        LOG_E("APP", "Persistent settings unavailable; defaults remain active");
    }
    Localization::setLanguage(settings_.viewState().uiLanguage);

    if (!Drivers::Display::begin()) {
        halt("Display initialization failed");
    }

    // Display initialization starts the shared board I2C bus. Initialize the
    // AXP2101 before the FT6336 touch driver so both clients keep using the
    // final Wire configuration selected by XPowersLib.
    if (!power_.begin()) {
        LOG_E("APP", "AXP2101 telemetry unavailable; terminal remains active");
    }

    if (!time_.begin(millis())) {
        LOG_E("APP", "RTC unavailable; clock will wait for GPS time");
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

    displayPower_.begin(millis(), settings_.viewState(), power_.viewState());

    if (!buttons_.begin()) {
        LOG_E("APP", "Onboard BOOT button is unavailable");
    }

    if (AppConfig::ENABLE_GPS) {
        gps_.begin();
    }

    // Arduino_GFX/VSPI is initialized before the independent LoRa HSPI bus.
    if (AppConfig::ENABLE_LORA && !radio_.begin(settings_.viewState())) {
        LOG_E("APP", "LoRa is unavailable; UI remains active");
    }

    tracker_.begin();
    trail_.begin();
    if (!map_.begin()) {
        LOG_E("APP", "Offline map framebuffer unavailable");
    }
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
        this,
        mapPanThunk,
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
    const bool touchActivity = Drivers::LvglPort::consumeTouchActivity();
    const bool buttonActivity = buttons_.consumePressActivity();

    if (AppConfig::ENABLE_GPS) {
        gps_.update(now);
    }
    time_.update(now, gps_.viewState());
    if (AppConfig::ENABLE_LORA) {
        radio_.update(now, settings_.viewState());
    }
    power_.update(now);

    const bool wokeFromActivity = displayPower_.update(
        now,
        settings_.viewState(),
        power_.viewState(),
        touchActivity || buttonActivity);
    if (wokeFromActivity && buttonActivity) {
        // A BOOT press used to wake the display must not also transmit a beacon.
        buttons_.suppressCurrentClick();
    }

    const bool bootBeaconRequested = buttons_.consumeBootClick();
    if (bootBeaconRequested) {
        tracker_.requestImmediateBeacon(now);
    }

    tracker_.update(now, settings_.viewState(), gps_.viewState(), radio_);
    trail_.update(now, settings_.viewState().trailEnabled, gps_.viewState());
    updateReferencePosition();
    astronomy_.update(
        now,
        time_.viewState().valid,
        time_.viewState().localYear,
        time_.viewState().localMonth,
        time_.viewState().localDay,
        time_.viewState().utcYear,
        time_.viewState().utcMonth,
        time_.viewState().utcDay,
        time_.viewState().utcHour,
        time_.viewState().utcMinute,
        time_.viewState().utcSecond,
        referencePosition_);
    map_.update(
        now,
        screens_.currentScreen() == App::ScreenId::Map,
        referencePosition_);

    screens_.update(
        now,
        radio_.viewState(),
        radio_.messageViewState(),
        radio_.stationViewState(),
        radio_.weatherViewState(),
        gps_.viewState(),
        tracker_.viewState(),
        trail_.viewState(),
        power_.viewState(),
        time_.viewState(),
        astronomy_.viewState(),
        radio_.digiIgateViewState(),
        referencePosition_,
        map_.viewState(),
        settings_.viewState());

    const Services::TrackerService::ViewState& trackerState = tracker_.viewState();
    if (trackerState.manualPacketsSent != observedManualPacketsSent_) {
        observedManualPacketsSent_ = trackerState.manualPacketsSent;
        screens_.setMessage(
            Localization::text(
                "BOOT: pozicni beacon byl zarazen do TX fronty.",
                "BOOT: position beacon was queued for transmission."));
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

void AppController::mapPanThunk(
    std::int16_t deltaX,
    std::int16_t deltaY,
    void* context) {

    if (context != nullptr) {
        static_cast<AppController*>(context)->map_.panByPixels(deltaX, deltaY);
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
    std::uint8_t batteryBrightnessPercent,
    std::uint16_t displayTimeoutSeconds,
    UiLanguage uiLanguage,
    LoRaPreset loraPreset,
    float loraFrequencyMHz,
    float loraBandwidthKHz,
    std::uint8_t loraSpreadingFactor,
    std::uint8_t loraCodingRate,
    std::int8_t loraOutputPowerDbm,
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
        batteryBrightnessPercent,
        displayTimeoutSeconds,
        uiLanguage,
        loraPreset,
        loraFrequencyMHz,
        loraBandwidthKHz,
        loraSpreadingFactor,
        loraCodingRate,
        loraOutputPowerDbm,
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
    bool trailEnabled,
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
        trailEnabled,
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
    std::uint8_t batteryBrightnessPercent,
    std::uint16_t displayTimeoutSeconds,
    UiLanguage uiLanguage,
    LoRaPreset loraPreset,
    float loraFrequencyMHz,
    float loraBandwidthKHz,
    std::uint8_t loraSpreadingFactor,
    std::uint8_t loraCodingRate,
    std::int8_t loraOutputPowerDbm,
    char* errorText,
    std::size_t errorTextCapacity) {

    const bool saved = settings_.save(
        callsign,
        latitude,
        longitude,
        batteryBrightnessPercent,
        displayTimeoutSeconds,
        uiLanguage,
        loraPreset,
        loraFrequencyMHz,
        loraBandwidthKHz,
        loraSpreadingFactor,
        loraCodingRate,
        loraOutputPowerDbm,
        errorText,
        errorTextCapacity);
    if (saved) {
        Localization::setLanguage(settings_.viewState().uiLanguage);
        if (AppConfig::ENABLE_LORA) {
            radio_.requestConfiguration(settings_.viewState());
        }
    }
    return saved;
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
            Localization::text(
                "DIGI/iGate nelze zapnout: radio neni inicializovano.",
                "DIGI/iGate cannot be enabled: radio is not initialized."));
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
    bool trailEnabled,
    TrackerPositionSource source,
    TrackerPositionFormat format,
    TrackerBeaconMode mode,
    TrackerSymbol symbol,
    std::uint32_t fixedIntervalSeconds,
    char* errorText,
    std::size_t errorTextCapacity) {

    const bool english = Localization::isEnglish();
    if (enabled && !radio_.viewState().initialized) {
        copyError(
            errorText,
            errorTextCapacity,
            english
                ? "Tracker cannot be enabled: radio is not initialized."
                : "Tracker nelze zapnout: radio neni inicializovano.");
        return false;
    }
    if (enabled && source == TrackerPositionSource::Gps &&
        !gps_.viewState().receiverDetected) {
        copyError(
            errorText,
            errorTextCapacity,
            english
                ? "Tracker cannot be enabled: GPS was not detected."
                : "Tracker nelze zapnout: GPS nebyla nalezena.");
        return false;
    }
    if (trailEnabled && !Drivers::SdCard::status().mounted) {
        copyError(
            errorText,
            errorTextCapacity,
            english
                ? "Trail logger cannot be enabled: SD card is unavailable."
                : "Stopar nelze zapnout: SD karta neni dostupna.");
        return false;
    }

    return settings_.saveTracker(
        enabled,
        trailEnabled,
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
        copyError(
            errorText,
            errorTextCapacity,
            Localization::text("Radio neni inicializovano.", "Radio is not initialized."));
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
            screens_.setMessage(Localization::text(
                "Radio neni inicializovano.",
                "Radio is not initialized."));
            return;
        }
        const bool started = radio_.sendTestPacket(settings_.viewState().callsign, millis());
        screens_.setMessage(started
            ? Localization::text(
                "Testovaci APRS paket byl zarazen do TX fronty.",
                "The test APRS packet was queued for transmission.")
            : Localization::text(
                "Test nelze zaradit: TX fronta je plna.",
                "The test packet cannot be queued: the TX queue is full."));
    } else if (command == Command::ToggleTrailPause) {
        char message[128] = {};
        trail_.toggleManualPause(
            millis(),
            gps_.viewState(),
            message,
            sizeof(message));
        screens_.setMessage(message);
    } else if (command == Command::MapZoomIn) {
        map_.zoomIn();
    } else if (command == Command::MapZoomOut) {
        map_.zoomOut();
    } else if (command == Command::MapRecenter) {
        map_.recenter();
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
