#pragma once

#include <cstdint>

namespace Services {

struct OtaViewState {
    bool enabled = false;
    bool accessPointActive = false;
    bool uploadActive = false;
    bool restartPending = false;
    bool manuallyStopped = false;
    std::uint8_t connectedClients = 0;
    std::uint32_t uploadedBytes = 0;
    std::uint32_t maximumFirmwareBytes = 0;
    std::uint32_t revision = 0;
    char statusText[128] = "OTA vypnuto";
};

}  // namespace Services
