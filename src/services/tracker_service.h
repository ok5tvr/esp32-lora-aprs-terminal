#pragma once

#include <cstdint>

#include "services/gps_service.h"
#include "services/radio_service.h"
#include "services/settings_service.h"

namespace Services {

class TrackerService {
public:
    struct ViewState {
        bool configuredEnabled = false;
        bool active = false;
        bool waitingForGps = false;
        bool positionValid = false;
        bool usingGps = false;
        bool smartBeacon = false;
        bool manualBeaconPending = false;
        double latitude = 0.0;
        double longitude = 0.0;
        float speedKmh = 0.0F;
        float courseDegrees = 0.0F;
        std::uint32_t nextTransmitSeconds = 0;
        std::uint32_t trackerPacketsSent = 0;
        std::uint32_t manualPacketsSent = 0;
        std::uint32_t manualBeaconFailures = 0;
        std::uint32_t lastTransmitAtMs = 0;
        std::uint32_t revision = 0;
        char statusText[96] = "Tracker vypnut";
        char lastFrame[192] = "Zatim nebyla odeslana poloha.";
    };

    void begin();
    void requestImmediateBeacon(std::uint32_t now);
    void update(
        std::uint32_t now,
        const SettingsService::ViewState& settings,
        const GpsService::ViewState& gps,
        RadioService& radio);
    const ViewState& viewState() const;

private:
    std::uint32_t intervalForSpeed(float speedKmh) const;
    bool cornerBeaconDue(
        std::uint32_t now,
        float speedKmh,
        float courseDegrees,
        bool courseValid) const;
    bool buildAndSend(
        std::uint32_t now,
        const SettingsService::ViewState& settings,
        const GpsService::ViewState& gps,
        RadioService& radio);
    void setStatus(const char* text);
    void failManualBeacon(const char* reason);

    ViewState view_;
    std::uint32_t lastSettingsRevision_ = 0xFFFFFFFFU;
    std::uint32_t armedAtMs_ = 0;
    std::uint32_t manualRequestedAtMs_ = 0;
    bool manualBeaconPending_ = false;
    bool hasTransmitted_ = false;
    float lastCourseAtTransmit_ = 0.0F;
    bool lastCourseValid_ = false;
};

}  // namespace Services
