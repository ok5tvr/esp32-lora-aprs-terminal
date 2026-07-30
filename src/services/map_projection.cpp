#include "services/map_projection.h"

#include <cmath>

namespace Services::MapProjection {
namespace {

constexpr double PI = 3.14159265358979323846;

}  // namespace

bool validCoordinate(double latitude, double longitude) {
    return std::isfinite(latitude) && std::isfinite(longitude) &&
        latitude >= -90.0 && latitude <= 90.0 &&
        longitude >= -180.0 && longitude <= 180.0;
}

double worldSize(std::uint8_t zoom) {
    if (zoom > 30U) {
        return 0.0;
    }
    return std::ldexp(static_cast<double>(TILE_SIZE), zoom);
}

WorldPixel toWorldPixel(double latitude, double longitude, std::uint8_t zoom) {
    WorldPixel result;
    if (!validCoordinate(latitude, longitude)) {
        return result;
    }

    const double size = worldSize(zoom);
    if (!(size > 0.0)) {
        return result;
    }

    if (latitude > MAX_MERCATOR_LATITUDE) {
        latitude = MAX_MERCATOR_LATITUDE;
    } else if (latitude < -MAX_MERCATOR_LATITUDE) {
        latitude = -MAX_MERCATOR_LATITUDE;
    }

    const double latitudeRadians = latitude * PI / 180.0;
    const double sine = std::sin(latitudeRadians);
    result.x = (longitude + 180.0) / 360.0 * size;
    result.y = (0.5 - std::log((1.0 + sine) / (1.0 - sine)) / (4.0 * PI)) * size;
    result.worldSize = size;
    result.valid = std::isfinite(result.x) && std::isfinite(result.y);
    return result;
}

ScreenPoint projectToViewport(
    double latitude,
    double longitude,
    std::uint8_t zoom,
    double centerWorldX,
    double centerWorldY,
    std::uint16_t viewportWidth,
    std::uint16_t viewportHeight) {

    ScreenPoint result;
    const WorldPixel world = toWorldPixel(latitude, longitude, zoom);
    if (!world.valid || !std::isfinite(centerWorldX) || !std::isfinite(centerWorldY)) {
        return result;
    }

    double deltaX = world.x - centerWorldX;
    const double halfWorld = world.worldSize / 2.0;
    if (deltaX > halfWorld) {
        deltaX -= world.worldSize;
    } else if (deltaX < -halfWorld) {
        deltaX += world.worldSize;
    }

    result.x = static_cast<double>(viewportWidth) / 2.0 + deltaX;
    result.y = static_cast<double>(viewportHeight) / 2.0 + (world.y - centerWorldY);
    result.valid = std::isfinite(result.x) && std::isfinite(result.y);
    return result;
}

}  // namespace Services::MapProjection
