#include "services/ota_validation.h"

#include <cctype>
#include <cstring>

namespace Services {
namespace OtaValidation {

bool hasFirmwareBinExtension(const char* filename) {
    if (filename == nullptr) {
        return false;
    }
    const std::size_t length = std::strlen(filename);
    if (length < 5U) {
        return false;
    }
    const char* extension = filename + length - 4U;
    return extension[0] == '.' &&
        std::tolower(static_cast<unsigned char>(extension[1])) == 'b' &&
        std::tolower(static_cast<unsigned char>(extension[2])) == 'i' &&
        std::tolower(static_cast<unsigned char>(extension[3])) == 'n';
}

bool hasValidEsp32AppHeader(const std::uint8_t* data, std::size_t length) {
    if (data == nullptr || length < ESP32_APP_HEADER_BYTES) {
        return false;
    }

    // ESP image header: magic, sensible segment count, and supported flash mode.
    if (data[0] != 0xE9U || data[1] == 0U || data[1] > 16U || data[2] > 3U) {
        return false;
    }

    // The first segment starts after the 24-byte image header and 8-byte
    // segment header. A firmware application contains esp_app_desc_t there;
    // its magic word is 0xABCD5432 (little endian). This rejects bootloader,
    // partition-table and arbitrary .bin files before they reach Update.write().
    return data[32] == 0x32U && data[33] == 0x54U &&
        data[34] == 0xCDU && data[35] == 0xABU;
}

}  // namespace OtaValidation
}  // namespace Services
