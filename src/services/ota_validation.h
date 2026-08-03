#pragma once

#include <cstddef>
#include <cstdint>

namespace Services {
namespace OtaValidation {

constexpr std::size_t ESP32_APP_HEADER_BYTES = 36;

bool hasFirmwareBinExtension(const char* filename);
bool hasValidEsp32AppHeader(const std::uint8_t* data, std::size_t length);

}  // namespace OtaValidation
}  // namespace Services
