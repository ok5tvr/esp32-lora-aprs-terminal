#pragma once

#include <cstdint>

namespace Services::MapProjection {

constexpr std::uint16_t TILE_SIZE = 256;
constexpr double MAX_MERCATOR_LATITUDE = 85.05112877980659;

struct WorldPixel {
    bool valid = false;
    double x = 0.0;
    double y = 0.0;
    double worldSize = 0.0;
};

struct ScreenPoint {
    bool valid = false;
    double x = 0.0;
    double y = 0.0;
};

struct GeoCoordinate {
    bool valid = false;
    double latitude = 0.0;
    double longitude = 0.0;
};

bool validCoordinate(double latitude, double longitude);
double worldSize(std::uint8_t zoom);
WorldPixel toWorldPixel(double latitude, double longitude, std::uint8_t zoom);
GeoCoordinate fromWorldPixel(double x, double y, std::uint8_t zoom);
ScreenPoint projectToViewport(
    double latitude,
    double longitude,
    std::uint8_t zoom,
    double centerWorldX,
    double centerWorldY,
    std::uint16_t viewportWidth,
    std::uint16_t viewportHeight);

}  // namespace Services::MapProjection
