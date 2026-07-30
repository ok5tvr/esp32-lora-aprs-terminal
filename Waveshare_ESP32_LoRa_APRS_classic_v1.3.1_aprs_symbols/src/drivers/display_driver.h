#pragma once

#include <cstdint>

namespace Drivers {
namespace Display {

bool begin();
void drawRgb565Bitmap(
    std::int16_t x,
    std::int16_t y,
    std::uint16_t* pixels,
    std::uint16_t width,
    std::uint16_t height,
    bool byteSwapped);

}  // namespace Display
}  // namespace Drivers
