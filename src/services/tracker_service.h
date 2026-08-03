#pragma once

#include <cstdint>

#include "app/smartbeacon_profiles.h"
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
        bool txPending = false;
        App::SmartBeaconProfile smartProfile = App::SmartBeaconProfile::Car;
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
        char statusText[96] = "--";
        char lastBeaconReason[32] = "--";
        char lastFrame[192] = "--";
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
    enum class BeaconReason : std::uint8_t {
        None,
        Initial,
        FixedInterval,
        SpeedInterval,
        Corner,
        StartMoving,
        Stopped,
        Manual
    };

    std::uint32_t intervalForSpeed(
        float speedKmh,
        const App::SmartBeaconProfileDefinition& profile) const;
    bool cornerBeaconDue(
        std::uint32_t now,
        float speedKmh,
        float courseDegrees,
        bool courseValid,
        const App::SmartBeaconProfileDefinition& profile) const;
    BeaconReason motionBeaconDue(
        std::uint32_t now,
        float speedKmh,
        bool speedValid,
        const App::SmartBeaconProfileDefinition& profile);
    bool buildAndQueue(
        std::uint32_t now,
        BeaconReason reason,
        const SettingsService::ViewState& settings,
        const GpsService::ViewState& gps,
        RadioService& radio);
    void processTransmissionResult(
        std::uint32_t now,
        const SettingsService::ViewState& settings,
        const RadioService::ViewState& radio);
    void confirmTransmission(
        std::uint32_t now,
        const SettingsService::ViewState& settings);
    void failPendingTransmission(bool timeout);
    const char* reasonText(BeaconReason reason) const;
    void setStatus(const char* text);
    void setStatusLocalized(const char* czech, const char* english);
    void failManualBeaconLocalized(const char* czech, const char* english);

    ViewState view_;
    std::uint32_t lastSettingsRevision_ = 0xFFFFFFFFU;
    std::uint32_t armedAtMs_ = 0;
    std::uint32_t retryNotBeforeMs_ = 0;
    std::uint32_t manualRequestedAtMs_ = 0;
    bool manualBeaconPending_ = false;
    bool hasTransmitted_ = false;
    App::UiLanguage language_ = App::UiLanguage::Czech;
    float lastCourseAtTransmit_ = 0.0F;
    bool lastCourseValid_ = false;
    bool moving_ = false;
    std::uint32_t startCandidateAtMs_ = 0;
    std::uint32_t stopCandidateAtMs_ = 0;

    bool pendingTx_ = false;
    bool pendingManual_ = false;
    std::uint32_t pendingTxSequence_ = 0;
    std::uint32_t pendingQueuedAtMs_ = 0;
    BeaconReason pendingReason_ = BeaconReason::None;
    float pendingCourseDegrees_ = 0.0F;
    float pendingSpeedKmh_ = 0.0F;
    bool pendingCourseValid_ = false;
    bool pendingSpeedValid_ = false;
    std::uint32_t observedTxCompletionRevision_ = 0xFFFFFFFFU;
    std::uint32_t observedTxFailureRevision_ = 0xFFFFFFFFU;
};

}  // namespace Services
