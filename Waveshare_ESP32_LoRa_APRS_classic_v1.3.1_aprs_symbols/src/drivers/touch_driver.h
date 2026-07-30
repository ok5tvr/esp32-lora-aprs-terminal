#pragma once

#include <cstdint>

namespace Drivers {
namespace Touch {

bool begin();
bool readPoint(std::int16_t& x, std::int16_t& y);

}  // namespace Touch
}  // namespace Drivers
