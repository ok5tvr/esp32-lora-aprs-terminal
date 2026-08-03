#pragma once

#include <cmath>
#include <cstdint>

#include "app/app_types.h"

namespace App {

struct SmartBeaconProfileDefinition {
    float lowSpeedKmh;
    float highSpeedKmh;
    std::uint32_t slowRateSeconds;
    std::uint32_t fastRateSeconds;
    std::uint32_t minTurnSeconds;
    float turnAngleDegrees;
    float turnSlope;
    float startSpeedKmh;
    float stopSpeedKmh;
    std::uint32_t startConfirmSeconds;
    std::uint32_t stopConfirmSeconds;
};

inline bool validSmartBeaconProfile(SmartBeaconProfile profile) {
    return static_cast<std::uint8_t>(profile) <=
        static_cast<std::uint8_t>(SmartBeaconProfile::Walking);
}

inline const SmartBeaconProfileDefinition& smartBeaconProfileDefinition(
    SmartBeaconProfile profile) {

    // These profiles are tuned for LoRa APRS operation: they react promptly
    // to movement and turns without producing excessive RF traffic.
    static constexpr SmartBeaconProfileDefinition CAR = {
        5.0F, 70.0F, 1800U, 120U, 15U, 30.0F, 240.0F,
        6.0F, 3.0F, 8U, 45U
    };
    static constexpr SmartBeaconProfileDefinition BICYCLE = {
        3.0F, 30.0F, 1200U, 90U, 15U, 35.0F, 160.0F,
        4.0F, 2.0F, 6U, 30U
    };
    static constexpr SmartBeaconProfileDefinition WALKING = {
        1.5F, 7.0F, 900U, 120U, 30U, 55.0F, 80.0F,
        2.0F, 0.7F, 10U, 60U
    };

    switch (profile) {
        case SmartBeaconProfile::Bicycle: return BICYCLE;
        case SmartBeaconProfile::Walking: return WALKING;
        case SmartBeaconProfile::Car:
        default: return CAR;
    }
}

inline std::uint32_t smartBeaconIntervalSeconds(
    const SmartBeaconProfileDefinition& profile,
    float speedKmh) {

    if (!std::isfinite(speedKmh) || speedKmh <= profile.lowSpeedKmh) {
        return profile.slowRateSeconds;
    }
    if (speedKmh >= profile.highSpeedKmh) {
        return profile.fastRateSeconds;
    }
    const float calculated =
        static_cast<float>(profile.fastRateSeconds) * profile.highSpeedKmh / speedKmh;
    std::uint32_t interval = static_cast<std::uint32_t>(calculated + 0.5F);
    if (interval < profile.fastRateSeconds) {
        interval = profile.fastRateSeconds;
    }
    if (interval > profile.slowRateSeconds) {
        interval = profile.slowRateSeconds;
    }
    return interval;
}

inline float smartBeaconTurnThresholdDegrees(
    const SmartBeaconProfileDefinition& profile,
    float speedKmh) {

    const float safeSpeed = std::isfinite(speedKmh) && speedKmh > 1.0F
        ? speedKmh
        : 1.0F;
    return profile.turnAngleDegrees + profile.turnSlope / safeSpeed;
}

inline const char* smartBeaconProfileLabel(
    SmartBeaconProfile profile,
    UiLanguage language) {

    switch (profile) {
        case SmartBeaconProfile::Bicycle:
            return language == UiLanguage::English ? "BICYCLE" : "KOLO";
        case SmartBeaconProfile::Walking:
            return language == UiLanguage::English ? "WALKING" : "CHUZE";
        case SmartBeaconProfile::Car:
        default:
            return language == UiLanguage::English ? "CAR" : "AUTO";
    }
}

}  // namespace App
