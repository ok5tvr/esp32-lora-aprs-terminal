#pragma once

#include <cstddef>
#include <cstdint>

#include "services/geo_utils.h"

namespace Ui {
namespace MenuScreen {

struct IndicatorState {
    bool gpsSerialTraffic = false;
    bool gpsNmeaPacket = false;
    bool gpsReceiverDetected = false;
    bool gpsFix = false;
    bool trackerConfigured = false;
    bool trackerActive = false;
    bool trailConfigured = false;
    bool trailRecording = false;
    bool trailPaused = false;
    bool trailError = false;
    bool digiEnabled = false;
    bool igateEnabled = false;
    bool igateVerified = false;
    std::uint8_t unreadMessages = 0;
    std::uint8_t newStations = 0;
};

void create(
    std::size_t selectedIndex,
    const Services::PositionReference& reference,
    const IndicatorState& indicators);
void update(
    const Services::PositionReference& reference,
    const IndicatorState& indicators);
void setMessage(const char* text);
void moveSelection(int direction);
std::size_t selectedIndex();

}  // namespace MenuScreen
}  // namespace Ui
