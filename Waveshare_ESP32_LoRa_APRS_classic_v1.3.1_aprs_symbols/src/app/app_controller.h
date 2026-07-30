#pragma once

#include <cstddef>
#include <cstdint>

#include "app/app_types.h"
#include "drivers/button_driver.h"
#include "services/geo_utils.h"
#include "services/gps_service.h"
#include "services/power_service.h"
#include "services/radio_service.h"
#include "services/settings_service.h"
#include "services/trail_service.h"
#include "services/tracker_service.h"
#include "ui/screen_manager.h"

namespace App {

class AppController {
public:
    bool begin();
    void update();

private:
    static void commandThunk(Command command, void* context);
    static bool messageSendThunk(
        const char* recipient,
        const char* text,
        char* errorText,
        std::size_t errorTextCapacity,
        void* context);
    static bool settingsSaveThunk(
        const char* callsign,
        double latitude,
        double longitude,
        char* errorText,
        std::size_t errorTextCapacity,
        void* context);
    static bool digiIgateSettingsSaveThunk(
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
        void* context);
    static bool trackerSettingsSaveThunk(
        bool enabled,
        bool trailEnabled,
        TrackerPositionSource source,
        TrackerPositionFormat format,
        TrackerBeaconMode mode,
        TrackerSymbol symbol,
        std::uint32_t fixedIntervalSeconds,
        char* errorText,
        std::size_t errorTextCapacity,
        void* context);
    void handleCommand(Command command);
    bool sendMessage(
        const char* recipient,
        const char* text,
        char* errorText,
        std::size_t errorTextCapacity);
    bool saveSettings(
        const char* callsign,
        double latitude,
        double longitude,
        char* errorText,
        std::size_t errorTextCapacity);
    bool saveDigiIgateSettings(
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
        std::size_t errorTextCapacity);
    bool saveTrackerSettings(
        bool enabled,
        bool trailEnabled,
        TrackerPositionSource source,
        TrackerPositionFormat format,
        TrackerBeaconMode mode,
        TrackerSymbol symbol,
        std::uint32_t fixedIntervalSeconds,
        char* errorText,
        std::size_t errorTextCapacity);
    void updateReferencePosition();
    [[noreturn]] void halt(const char* reason);

    Drivers::ButtonDriver buttons_;
    Services::SettingsService settings_;
    Services::GpsService gps_;
    Services::PowerService power_;
    Services::RadioService radio_;
    Services::TrackerService tracker_;
    Services::TrailService trail_;
    Services::PositionReference referencePosition_;
    Ui::ScreenManager screens_;
    std::uint32_t observedManualPacketsSent_ = 0;
    std::uint32_t observedManualBeaconFailures_ = 0;
};

}  // namespace App
