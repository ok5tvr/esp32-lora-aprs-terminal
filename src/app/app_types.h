#pragma once

#include <cstddef>
#include <cstdint>

namespace App {

enum class ScreenId : std::uint8_t {
    Splash,
    MainMenu,
    LoRaStatus,
    Diagnostics,
    Messages,
    GpsStatus,
    Astronomy,
    Map,
    Stations,
    StationDetail,
    StationNavigation,
    Weather,
    WeatherDetail,
    Tracker,
    Trail,
    Power,
    DigiIgate,
    Settings
};

enum class NavigationAction : std::uint8_t {
    Up,
    Down,
    Confirm,
    Back
};

enum class Command : std::uint8_t {
    None,
    SendTestPacket,
    ToggleTrailPause,
    MapZoomIn,
    MapZoomOut,
    MapRecenter
};

enum class TrackerPositionSource : std::uint8_t {
    Gps = 0,
    DefaultPosition = 1
};

enum class TrackerPositionFormat : std::uint8_t {
    Uncompressed = 0,
    Compressed = 1
};

enum class TrackerBeaconMode : std::uint8_t {
    FixedInterval = 0,
    SmartBeacon = 1
};

enum class DigiMode : std::uint8_t {
    FillInWide1 = 0,
    TraceWide2 = 1,
    FillInAndWide2 = 2
};

enum class LoRaPreset : std::uint8_t {
    CzeAprs = 0,
    Custom = 1
};

enum class UiLanguage : std::uint8_t {
    Czech = 0,
    English = 1
};

enum class TrackerSymbol : std::uint8_t {
    Car = 0,
    Person,
    Bicycle,
    Motorcycle,
    House,
    Boat,
    Aircraft,
    Balloon,
    Weather,
    Generic,
    LoraIgate,
    Count
};

using CommandHandler = void (*)(Command command, void* context);
using MapPanHandler = void (*)(std::int16_t deltaX, std::int16_t deltaY, void* context);
using MessageSendHandler = bool (*)(
    const char* recipient,
    const char* text,
    char* errorText,
    std::size_t errorTextCapacity,
    void* context);
using SettingsSaveHandler = bool (*)(
    const char* callsign,
    double latitude,
    double longitude,
    std::uint8_t batteryBrightnessPercent,
    std::uint16_t displayTimeoutSeconds,
    UiLanguage uiLanguage,
    bool otaEnabled,
    LoRaPreset loraPreset,
    float loraFrequencyMHz,
    float loraBandwidthKHz,
    std::uint8_t loraSpreadingFactor,
    std::uint8_t loraCodingRate,
    std::int8_t loraOutputPowerDbm,
    char* errorText,
    std::size_t errorTextCapacity,
    void* context);
using DigiIgateSettingsSaveHandler = bool (*)(
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
using TrackerSettingsSaveHandler = bool (*)(
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

}  // namespace App
