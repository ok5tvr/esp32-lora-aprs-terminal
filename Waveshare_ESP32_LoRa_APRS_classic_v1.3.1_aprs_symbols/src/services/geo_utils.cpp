#include "services/geo_utils.h"

#include <cmath>

namespace Services {
namespace {

constexpr double PI = 3.14159265358979323846;
constexpr double EARTH_RADIUS_KM = 6371.0088;

double toRadians(double degrees) {
    return degrees * PI / 180.0;
}

double toDegrees(double radians) {
    return radians * 180.0 / PI;
}

bool validCoordinate(double latitude, double longitude) {
    return std::isfinite(latitude) && std::isfinite(longitude) &&
        latitude >= -90.0 && latitude <= 90.0 &&
        longitude >= -180.0 && longitude <= 180.0;
}

double clampLocatorCoordinate(double value, double maximum) {
    if (value < 0.0) {
        return 0.0;
    }
    // The north/east edge belongs to the last valid Maidenhead square.
    if (value >= maximum) {
        return maximum - 0.000000001;
    }
    return value;
}

}  // namespace

DistanceBearing calculateDistanceBearing(
    double fromLatitude,
    double fromLongitude,
    double toLatitude,
    double toLongitude) {

    DistanceBearing result;
    if (!validCoordinate(fromLatitude, fromLongitude) ||
        !validCoordinate(toLatitude, toLongitude)) {
        return result;
    }

    const double lat1 = toRadians(fromLatitude);
    const double lat2 = toRadians(toLatitude);
    const double deltaLat = toRadians(toLatitude - fromLatitude);
    const double deltaLon = toRadians(toLongitude - fromLongitude);

    const double sinLat = std::sin(deltaLat / 2.0);
    const double sinLon = std::sin(deltaLon / 2.0);
    const double a = sinLat * sinLat +
        std::cos(lat1) * std::cos(lat2) * sinLon * sinLon;
    const double clampedA = a < 0.0 ? 0.0 : (a > 1.0 ? 1.0 : a);
    const double centralAngle = 2.0 * std::atan2(
        std::sqrt(clampedA), std::sqrt(1.0 - clampedA));

    const double y = std::sin(deltaLon) * std::cos(lat2);
    const double x = std::cos(lat1) * std::sin(lat2) -
        std::sin(lat1) * std::cos(lat2) * std::cos(deltaLon);
    double bearing = toDegrees(std::atan2(y, x));
    if (bearing < 0.0) {
        bearing += 360.0;
    }
    if (bearing >= 360.0) {
        bearing -= 360.0;
    }

    result.valid = true;
    result.distanceKm = EARTH_RADIUS_KM * centralAngle;
    result.bearingDegrees = bearing;
    return result;
}

const char* cardinalDirection(double bearingDegrees) {
    static constexpr const char* DIRECTIONS[16] = {
        "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
    };
    if (!std::isfinite(bearingDegrees)) {
        return "--";
    }
    double normalized = std::fmod(bearingDegrees, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    const int index = static_cast<int>(std::floor((normalized + 11.25) / 22.5)) & 15;
    return DIRECTIONS[index];
}

bool maidenheadLocator(
    double latitude,
    double longitude,
    char* output,
    std::size_t outputCapacity) {

    if (output == nullptr || outputCapacity < 7U) {
        return false;
    }
    output[0] = '\0';

    if (!validCoordinate(latitude, longitude)) {
        return false;
    }

    double adjustedLongitude = clampLocatorCoordinate(longitude + 180.0, 360.0);
    double adjustedLatitude = clampLocatorCoordinate(latitude + 90.0, 180.0);

    const int fieldLongitude = static_cast<int>(std::floor(adjustedLongitude / 20.0));
    const int fieldLatitude = static_cast<int>(std::floor(adjustedLatitude / 10.0));

    adjustedLongitude -= static_cast<double>(fieldLongitude) * 20.0;
    adjustedLatitude -= static_cast<double>(fieldLatitude) * 10.0;

    const int squareLongitude = static_cast<int>(std::floor(adjustedLongitude / 2.0));
    const int squareLatitude = static_cast<int>(std::floor(adjustedLatitude));

    adjustedLongitude -= static_cast<double>(squareLongitude) * 2.0;
    adjustedLatitude -= static_cast<double>(squareLatitude);

    int subsquareLongitude = static_cast<int>(std::floor(adjustedLongitude * 12.0));
    int subsquareLatitude = static_cast<int>(std::floor(adjustedLatitude * 24.0));
    if (subsquareLongitude < 0) subsquareLongitude = 0;
    if (subsquareLongitude > 23) subsquareLongitude = 23;
    if (subsquareLatitude < 0) subsquareLatitude = 0;
    if (subsquareLatitude > 23) subsquareLatitude = 23;

    output[0] = static_cast<char>('A' + fieldLongitude);
    output[1] = static_cast<char>('A' + fieldLatitude);
    output[2] = static_cast<char>('0' + squareLongitude);
    output[3] = static_cast<char>('0' + squareLatitude);
    output[4] = static_cast<char>('A' + subsquareLongitude);
    output[5] = static_cast<char>('A' + subsquareLatitude);
    output[6] = '\0';
    return true;
}

}  // namespace Services
