#pragma once

#include <cstddef>
#include <cstdint>

namespace App {

enum class ScreenId : std::uint8_t {
    Splash,
    MainMenu,
    LoRaStatus,
    Messages,
    GpsStatus,
    Stations,
    Weather,
    Tracker,
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
    SendTestPacket
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
    TrackerPositionSource source,
    TrackerPositionFormat format,
    TrackerBeaconMode mode,
    TrackerSymbol symbol,
    std::uint32_t fixedIntervalSeconds,
    char* errorText,
    std::size_t errorTextCapacity,
    void* context);

}  // namespace App
