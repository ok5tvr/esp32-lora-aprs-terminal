#pragma once

#include <cstddef>
#include <cstdint>

namespace Services {

struct PositionReference {
    bool valid = false;
    bool fromGps = false;
    double latitude = 0.0;
    double longitude = 0.0;
    std::uint32_t revision = 0;
};

struct DistanceBearing {
    bool valid = false;
    double distanceKm = 0.0;
    double bearingDegrees = 0.0;
};

DistanceBearing calculateDistanceBearing(
    double fromLatitude,
    double fromLongitude,
    double toLatitude,
    double toLongitude);

const char* cardinalDirection(double bearingDegrees);

// Converts WGS-84 coordinates to a six-character Maidenhead locator.
// Example: 49.786333 N, 13.285000 E -> JN69PS.
bool maidenheadLocator(
    double latitude,
    double longitude,
    char* output,
    std::size_t outputCapacity);

}  // namespace Services
