#include <cassert>
#include <cstdint>
#include <iostream>

#include "services/ota_validation.h"

int main() {
    using namespace Services::OtaValidation;

    assert(hasFirmwareBinExtension("firmware.bin"));
    assert(hasFirmwareBinExtension("release.BIN"));
    assert(!hasFirmwareBinExtension("firmware.bin.zip"));
    assert(!hasFirmwareBinExtension("partitions.csv"));
    assert(!hasFirmwareBinExtension(nullptr));

    std::uint8_t valid[ESP32_APP_HEADER_BYTES] = {};
    valid[0] = 0xE9;
    valid[1] = 4;
    valid[2] = 2;
    valid[32] = 0x32;
    valid[33] = 0x54;
    valid[34] = 0xCD;
    valid[35] = 0xAB;
    assert(hasValidEsp32AppHeader(valid, sizeof(valid)));

    valid[0] = 0x00;
    assert(!hasValidEsp32AppHeader(valid, sizeof(valid)));
    valid[0] = 0xE9;
    valid[32] = 0x00;
    assert(!hasValidEsp32AppHeader(valid, sizeof(valid)));
    assert(!hasValidEsp32AppHeader(valid, 20));

    std::cout << "OTA validation tests passed\n";
    return 0;
}
