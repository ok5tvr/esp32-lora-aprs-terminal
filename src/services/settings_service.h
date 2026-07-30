#pragma once

#include <cstddef>
#include <cstdint>

#include "app/app_types.h"

namespace Services {

class SettingsService {
public:
    static constexpr std::size_t CALLSIGN_CAPACITY = 16;
    static constexpr std::size_t WIFI_SSID_CAPACITY = 33;
    static constexpr std::size_t WIFI_PASSWORD_CAPACITY = 65;
    static constexpr std::size_t APRS_IS_SERVER_CAPACITY = 64;
    static constexpr std::size_t APRS_IS_FILTER_CAPACITY = 80;

    struct ViewState {
        char callsign[CALLSIGN_CAPACITY] = {};
        double defaultLatitude = 0.0;
        double defaultLongitude = 0.0;
        bool trackerEnabled = false;
        bool trailEnabled = false;
        App::TrackerPositionSource trackerSource = App::TrackerPositionSource::Gps;
        App::TrackerPositionFormat trackerFormat = App::TrackerPositionFormat::Uncompressed;
        App::TrackerBeaconMode trackerMode = App::TrackerBeaconMode::FixedInterval;
        App::TrackerSymbol trackerSymbol = App::TrackerSymbol::Car;
        std::uint32_t trackerFixedIntervalSeconds = 300;
        bool digiEnabled = false;
        App::DigiMode digiMode = App::DigiMode::FillInWide1;
        std::uint8_t digiMaxWideHops = 2;
        bool igateEnabled = false;
        char wifiSsid[WIFI_SSID_CAPACITY] = {};
        char wifiPassword[WIFI_PASSWORD_CAPACITY] = {};
        char aprsIsServer[APRS_IS_SERVER_CAPACITY] = {};
        std::uint16_t aprsIsPort = 14580;
        std::int32_t aprsIsPasscode = -1;
        char aprsIsFilter[APRS_IS_FILTER_CAPACITY] = {};
        std::uint32_t revision = 0;
        bool persistentStorageReady = false;
    };

    bool begin();
    bool save(
        const char* callsign,
        double latitude,
        double longitude,
        char* errorText,
        std::size_t errorTextCapacity);
    bool saveTracker(
        bool enabled,
        bool trailEnabled,
        App::TrackerPositionSource source,
        App::TrackerPositionFormat format,
        App::TrackerBeaconMode mode,
        App::TrackerSymbol symbol,
        std::uint32_t fixedIntervalSeconds,
        char* errorText,
        std::size_t errorTextCapacity);
    bool saveDigiIgate(
        bool digiEnabled,
        App::DigiMode digiMode,
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
    const ViewState& viewState() const;

private:
    static bool normalizeCallsign(
        const char* input,
        char* output,
        std::size_t outputCapacity);
    static void setError(
        char* output,
        std::size_t outputCapacity,
        const char* text);

    ViewState view_;
};

}  // namespace Services
