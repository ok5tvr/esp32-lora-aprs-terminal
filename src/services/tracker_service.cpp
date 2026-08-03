#include "services/tracker_service.h"

#include <Arduino.h>
#include <aprs_codec.h>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "app/aprs_path.h"
#include "app/tracker_symbols.h"
#include "app_config.h"
#include "app_log.h"

namespace Services {
namespace {

float angularDifference(float first, float second) {
    float difference = std::fmod(std::fabs(first - second), 360.0F);
    if (difference > 180.0F) {
        difference = 360.0F - difference;
    }
    return difference;
}

bool timeReached(std::uint32_t now, std::uint32_t target) {
    return static_cast<std::int32_t>(now - target) >= 0;
}

}  // namespace

void TrackerService::begin() {
    view_ = ViewState{};
    lastSettingsRevision_ = 0xFFFFFFFFU;
    armedAtMs_ = 0;
    retryNotBeforeMs_ = 0;
    manualRequestedAtMs_ = 0;
    manualBeaconPending_ = false;
    hasTransmitted_ = false;
    lastCourseValid_ = false;
    moving_ = false;
    startCandidateAtMs_ = 0;
    stopCandidateAtMs_ = 0;
    pendingTx_ = false;
    pendingManual_ = false;
    pendingTxSequence_ = 0;
    pendingQueuedAtMs_ = 0;
    pendingReason_ = BeaconReason::None;
    pendingCourseValid_ = false;
    pendingSpeedValid_ = false;
    observedTxCompletionRevision_ = 0xFFFFFFFFU;
    observedTxFailureRevision_ = 0xFFFFFFFFU;
    language_ = App::UiLanguage::Czech;
}

void TrackerService::requestImmediateBeacon(std::uint32_t now) {
    if (manualBeaconPending_) {
        LOG_I("TRACKER", "Manual beacon request already pending");
        return;
    }

    manualBeaconPending_ = true;
    manualRequestedAtMs_ = now;
    view_.manualBeaconPending = true;
    setStatusLocalized(
        "BOOT beacon: pozadavek prijat",
        "BOOT beacon: request accepted");
    LOG_I("TRACKER", "Manual position beacon requested by BOOT button");
}

void TrackerService::update(
    std::uint32_t now,
    const SettingsService::ViewState& settings,
    const GpsService::ViewState& gps,
    RadioService& radio) {

    language_ = settings.uiLanguage;
    processTransmissionResult(now, settings, radio.viewState());

    if (lastSettingsRevision_ != settings.revision) {
        lastSettingsRevision_ = settings.revision;
        view_.configuredEnabled = settings.trackerEnabled;
        view_.smartBeacon = settings.trackerMode == App::TrackerBeaconMode::SmartBeacon;
        view_.smartProfile = settings.trackerSmartProfile;

        // Re-arm scheduled tracking after every saved configuration change.
        // A frame already accepted by the TX queue remains tracked until the
        // radio reports completion or failure.
        hasTransmitted_ = false;
        lastCourseValid_ = false;
        moving_ = false;
        startCandidateAtMs_ = 0;
        stopCandidateAtMs_ = 0;
        view_.nextTransmitSeconds = 0;
        if (settings.trackerEnabled) {
            armedAtMs_ = now + AppConfig::TRACKER_START_DELAY_MS;
        }
        ++view_.revision;
    }

    view_.configuredEnabled = settings.trackerEnabled;
    view_.smartBeacon = settings.trackerMode == App::TrackerBeaconMode::SmartBeacon;
    view_.smartProfile = settings.trackerSmartProfile;
    view_.usingGps = settings.trackerSource == App::TrackerPositionSource::Gps;
    view_.manualBeaconPending = manualBeaconPending_;
    view_.txPending = pendingTx_;
    view_.waitingForGps = false;
    view_.positionValid = false;
    view_.active = false;

    bool speedValid = false;
    bool courseValid = false;
    bool positionReady = true;

    if (settings.trackerSource == App::TrackerPositionSource::Gps) {
        if (!gps.receiverDetected || !gps.hasFix) {
            positionReady = false;
            view_.waitingForGps = settings.trackerEnabled || manualBeaconPending_;
        } else {
            view_.latitude = gps.latitude;
            view_.longitude = gps.longitude;
            view_.speedKmh = gps.speedKmh;
            view_.courseDegrees = gps.courseDegrees;
            speedValid = gps.speedValid;
            courseValid = gps.courseValid;
        }
    } else {
        view_.latitude = settings.defaultLatitude;
        view_.longitude = settings.defaultLongitude;
        view_.speedKmh = 0.0F;
        view_.courseDegrees = 0.0F;
    }

    view_.positionValid = positionReady;

    if (pendingTx_) {
        view_.nextTransmitSeconds = 0;
        setStatusLocalized(
            pendingManual_
                ? "BOOT beacon ceka na potvrzeni RF vysilani"
                : "Tracker ceka na potvrzeni RF vysilani",
            pendingManual_
                ? "BOOT beacon is waiting for RF TX confirmation"
                : "Tracker is waiting for RF TX confirmation");
        return;
    }

    // A short BOOT-button click requests one position packet even when the
    // periodic tracker is disabled. It is considered sent only after the
    // radio reports a completed transmission.
    if (manualBeaconPending_) {
        view_.nextTransmitSeconds = 0;

        if (now - manualRequestedAtMs_ > AppConfig::MANUAL_BEACON_REQUEST_TIMEOUT_MS) {
            failManualBeaconLocalized(
                "BOOT beacon selhal: vyprsel cas cekani",
                "BOOT beacon failed: request timed out");
            return;
        }
        if (!radio.viewState().initialized) {
            failManualBeaconLocalized(
                "BOOT beacon selhal: radio neni inicializovano",
                "BOOT beacon failed: radio is not initialized");
            return;
        }
        if (!positionReady) {
            if (gps.receiverDetected) {
                setStatusLocalized(
                    "BOOT beacon ceka na GPS fix",
                    "BOOT beacon is waiting for a GPS fix");
            } else {
                setStatusLocalized(
                    "BOOT beacon ceka: GPS nebyla nalezena",
                    "BOOT beacon is waiting: GPS not detected");
            }
            return;
        }
        if (buildAndQueue(now, BeaconReason::Manual, settings, gps, radio)) {
            setStatusLocalized(
                "BOOT beacon zarazen; cekam na dokonceni TX",
                "BOOT beacon queued; waiting for TX completion");
        } else {
            setStatusLocalized(
                "BOOT beacon ceka: odeslani zatim nebylo mozne",
                "BOOT beacon is waiting: transmission is not possible yet");
        }
        return;
    }

    if (!settings.trackerEnabled) {
        view_.nextTransmitSeconds = 0;
        setStatusLocalized(
            "Tracker vypnut; BOOT odesle jednorazovy beacon",
            "Tracker off; BOOT sends a one-time beacon");
        return;
    }

    if (!radio.viewState().initialized) {
        view_.nextTransmitSeconds = 0;
        setStatusLocalized(
            "Tracker ceka: radio neni inicializovano",
            "Tracker waiting: radio is not initialized");
        return;
    }

    if (!positionReady) {
        view_.nextTransmitSeconds = 0;
        if (gps.receiverDetected) {
            setStatusLocalized(
                "Tracker ceka na platny GPS fix",
                "Tracker is waiting for a valid GPS fix");
        } else {
            setStatusLocalized(
                "Tracker ceka: GPS nebyla nalezena",
                "Tracker waiting: GPS not detected");
        }
        return;
    }

    view_.active = true;
    if (!timeReached(now, retryNotBeforeMs_)) {
        view_.nextTransmitSeconds = (retryNotBeforeMs_ - now + 999U) / 1000U;
        return;
    }

    std::uint32_t intervalSeconds = settings.trackerFixedIntervalSeconds;
    BeaconReason dueReason = BeaconReason::None;

    if (!hasTransmitted_) {
        if (timeReached(now, armedAtMs_)) {
            dueReason = BeaconReason::Initial;
        } else {
            view_.nextTransmitSeconds = (armedAtMs_ - now + 999U) / 1000U;
        }
    } else if (settings.trackerMode == App::TrackerBeaconMode::SmartBeacon) {
        const App::SmartBeaconProfileDefinition& profile =
            App::smartBeaconProfileDefinition(settings.trackerSmartProfile);
        intervalSeconds = intervalForSpeed(speedValid ? view_.speedKmh : 0.0F, profile);

        dueReason = motionBeaconDue(
            now,
            speedValid ? view_.speedKmh : 0.0F,
            speedValid,
            profile);
        if (dueReason == BeaconReason::None && cornerBeaconDue(
                now,
                speedValid ? view_.speedKmh : 0.0F,
                view_.courseDegrees,
                courseValid,
                profile)) {
            dueReason = BeaconReason::Corner;
        }

        const std::uint32_t elapsedMs = now - view_.lastTransmitAtMs;
        const std::uint32_t intervalMs = intervalSeconds * 1000U;
        if (dueReason == BeaconReason::None && elapsedMs >= intervalMs) {
            dueReason = BeaconReason::SpeedInterval;
        }
        view_.nextTransmitSeconds = dueReason != BeaconReason::None || elapsedMs >= intervalMs
            ? 0
            : (intervalMs - elapsedMs + 999U) / 1000U;
    } else {
        const std::uint32_t elapsedMs = now - view_.lastTransmitAtMs;
        const std::uint32_t intervalMs = intervalSeconds * 1000U;
        if (elapsedMs >= intervalMs) {
            dueReason = BeaconReason::FixedInterval;
        }
        view_.nextTransmitSeconds = dueReason != BeaconReason::None
            ? 0
            : (intervalMs - elapsedMs + 999U) / 1000U;
    }

    if (dueReason != BeaconReason::None) {
        if (buildAndQueue(now, dueReason, settings, gps, radio)) {
            char status[96];
            std::snprintf(
                status,
                sizeof(status),
                language_ == App::UiLanguage::English
                    ? "Tracker queued: %s; waiting for TX"
                    : "Tracker zarazen: %s; cekam na TX",
                reasonText(dueReason));
            setStatus(status);
        } else {
            setStatusLocalized(
                "Tracker: paket se nepodarilo zaradit",
                "Tracker: packet could not be queued");
        }
        return;
    }

    char status[96];
    std::snprintf(
        status,
        sizeof(status),
        language_ == App::UiLanguage::English
            ? "Tracker active: next TX in %u s"
            : "Tracker aktivni: dalsi TX za %u s",
        static_cast<unsigned>(view_.nextTransmitSeconds));
    setStatus(status);
}

const TrackerService::ViewState& TrackerService::viewState() const {
    return view_;
}

std::uint32_t TrackerService::intervalForSpeed(
    float speedKmh,
    const App::SmartBeaconProfileDefinition& profile) const {

    return App::smartBeaconIntervalSeconds(profile, speedKmh);
}

bool TrackerService::cornerBeaconDue(
    std::uint32_t now,
    float speedKmh,
    float courseDegrees,
    bool courseValid,
    const App::SmartBeaconProfileDefinition& profile) const {

    if (!hasTransmitted_ || !moving_ || !lastCourseValid_ || !courseValid ||
        speedKmh <= profile.lowSpeedKmh) {
        return false;
    }
    if (now - view_.lastTransmitAtMs < profile.minTurnSeconds * 1000U) {
        return false;
    }

    const float threshold = App::smartBeaconTurnThresholdDegrees(profile, speedKmh);
    return angularDifference(courseDegrees, lastCourseAtTransmit_) >= threshold;
}

TrackerService::BeaconReason TrackerService::motionBeaconDue(
    std::uint32_t now,
    float speedKmh,
    bool speedValid,
    const App::SmartBeaconProfileDefinition& profile) {

    if (!speedValid || !std::isfinite(speedKmh)) {
        startCandidateAtMs_ = 0;
        stopCandidateAtMs_ = 0;
        return BeaconReason::None;
    }

    if (!moving_) {
        stopCandidateAtMs_ = 0;
        if (speedKmh < profile.startSpeedKmh) {
            startCandidateAtMs_ = 0;
            return BeaconReason::None;
        }
        if (startCandidateAtMs_ == 0) {
            startCandidateAtMs_ = now;
            return BeaconReason::None;
        }
        if (now - startCandidateAtMs_ >= profile.startConfirmSeconds * 1000U) {
            return BeaconReason::StartMoving;
        }
        return BeaconReason::None;
    }

    startCandidateAtMs_ = 0;
    if (speedKmh > profile.stopSpeedKmh) {
        stopCandidateAtMs_ = 0;
        return BeaconReason::None;
    }
    if (stopCandidateAtMs_ == 0) {
        stopCandidateAtMs_ = now;
        return BeaconReason::None;
    }
    if (now - stopCandidateAtMs_ >= profile.stopConfirmSeconds * 1000U) {
        return BeaconReason::Stopped;
    }
    return BeaconReason::None;
}

bool TrackerService::buildAndQueue(
    std::uint32_t now,
    BeaconReason reason,
    const SettingsService::ViewState& settings,
    const GpsService::ViewState& gps,
    RadioService& radio) {

    const bool usingGps = settings.trackerSource == App::TrackerPositionSource::Gps;
    const App::SmartBeaconProfileDefinition& profile =
        App::smartBeaconProfileDefinition(settings.trackerSmartProfile);
    const float reliableCourseSpeed = settings.trackerMode == App::TrackerBeaconMode::SmartBeacon
        ? profile.lowSpeedKmh
        : 1.0F;
    const bool speedValid = usingGps && gps.speedValid;
    const bool includeCourseSpeed = speedValid && gps.courseValid &&
        gps.speedKmh > reliableCourseSpeed;
    const double speedKnots = includeCourseSpeed
        ? static_cast<double>(gps.speedKmh) / 1.852
        : 0.0;

    const App::TrackerSymbolDefinition& symbol =
        App::trackerSymbolDefinition(settings.trackerSymbol);

    char frame[192] = {};
    if (!Aprs::buildPositionTnc2(
            settings.callsign,
            AppConfig::APRS_DESTINATION,
            App::aprsPathTnc2(settings.trackerPath),
            view_.latitude,
            view_.longitude,
            symbol.table,
            symbol.code,
            settings.trackerFormat == App::TrackerPositionFormat::Compressed,
            includeCourseSpeed,
            view_.courseDegrees,
            speedKnots,
            settings.trackerComment,
            frame,
            sizeof(frame))) {
        LOG_E("TRACKER", "Position frame encoding failed");
        return false;
    }

    std::uint32_t sequence = 0;
    const bool manual = reason == BeaconReason::Manual;
    if (!radio.queueTrackerPacket(frame, manual, now, &sequence)) {
        return false;
    }

    pendingTx_ = true;
    pendingManual_ = manual;
    pendingTxSequence_ = sequence;
    pendingQueuedAtMs_ = now;
    pendingReason_ = reason;
    pendingCourseDegrees_ = view_.courseDegrees;
    pendingSpeedKmh_ = view_.speedKmh;
    pendingCourseValid_ = includeCourseSpeed;
    pendingSpeedValid_ = speedValid;
    view_.txPending = true;
    std::snprintf(view_.lastFrame, sizeof(view_.lastFrame), "%s", frame);
    ++view_.revision;
    LOG_I(
        "TRACKER",
        "Queued sequence %lu reason %s: %s",
        static_cast<unsigned long>(sequence),
        reasonText(reason),
        frame);
    return true;
}

void TrackerService::processTransmissionResult(
    std::uint32_t now,
    const SettingsService::ViewState& settings,
    const RadioService::ViewState& radio) {

    if (observedTxCompletionRevision_ == 0xFFFFFFFFU) {
        observedTxCompletionRevision_ = radio.txCompletionRevision;
    }
    if (observedTxFailureRevision_ == 0xFFFFFFFFU) {
        observedTxFailureRevision_ = radio.txFailureRevision;
    }

    if (radio.txCompletionRevision != observedTxCompletionRevision_) {
        observedTxCompletionRevision_ = radio.txCompletionRevision;
        if (pendingTx_ && radio.lastCompletedTxSequence == pendingTxSequence_) {
            confirmTransmission(now, settings);
        }
    }

    if (radio.txFailureRevision != observedTxFailureRevision_) {
        observedTxFailureRevision_ = radio.txFailureRevision;
        if (pendingTx_ && radio.lastFailedTxSequence == pendingTxSequence_) {
            failPendingTransmission(false);
        }
    }

    if (pendingTx_ && now - pendingQueuedAtMs_ > AppConfig::TRACKER_TX_CONFIRM_TIMEOUT_MS) {
        failPendingTransmission(true);
    }
}

void TrackerService::confirmTransmission(
    std::uint32_t now,
    const SettingsService::ViewState& settings) {

    const BeaconReason completedReason = pendingReason_;
    const bool completedManual = pendingManual_;
    const App::SmartBeaconProfileDefinition& profile =
        App::smartBeaconProfileDefinition(settings.trackerSmartProfile);

    pendingTx_ = false;
    pendingManual_ = false;
    pendingTxSequence_ = 0;
    pendingReason_ = BeaconReason::None;
    view_.txPending = false;
    hasTransmitted_ = true;
    view_.lastTransmitAtMs = now;
    retryNotBeforeMs_ = 0;
    ++view_.trackerPacketsSent;

    if (pendingCourseValid_ && pendingSpeedKmh_ > profile.lowSpeedKmh) {
        lastCourseAtTransmit_ = pendingCourseDegrees_;
        lastCourseValid_ = true;
    } else {
        lastCourseValid_ = false;
    }

    if (completedReason == BeaconReason::StartMoving) {
        moving_ = true;
    } else if (completedReason == BeaconReason::Stopped) {
        moving_ = false;
        lastCourseValid_ = false;
    } else if (completedReason == BeaconReason::Initial) {
        moving_ = pendingSpeedValid_ && pendingSpeedKmh_ >= profile.startSpeedKmh;
    } else if (settings.trackerMode == App::TrackerBeaconMode::SmartBeacon &&
               !moving_ && pendingSpeedValid_ &&
               pendingSpeedKmh_ >= profile.startSpeedKmh) {
        // A periodic or manual packet may coincide with the confirmed start
        // of movement. It already reports the new position, so do not emit a
        // redundant Start-moving packet a few seconds later.
        moving_ = true;
    }
    startCandidateAtMs_ = 0;
    stopCandidateAtMs_ = 0;

    std::snprintf(
        view_.lastBeaconReason,
        sizeof(view_.lastBeaconReason),
        "%s",
        reasonText(completedReason));

    if (completedManual) {
        manualBeaconPending_ = false;
        view_.manualBeaconPending = false;
        ++view_.manualPacketsSent;
        if (settings.trackerEnabled) {
            setStatusLocalized(
                "BOOT beacon odvysilan; plan trackeru restartovan",
                "BOOT beacon transmitted; tracker schedule restarted");
        } else {
            setStatusLocalized(
                "BOOT beacon odvysilan; tracker zustava vypnut",
                "BOOT beacon transmitted; tracker remains off");
        }
    } else {
        char status[96];
        std::snprintf(
            status,
            sizeof(status),
            language_ == App::UiLanguage::English
                ? "Tracker TX completed: %s"
                : "Tracker TX dokoncen: %s",
            reasonText(completedReason));
        setStatus(status);
    }
    ++view_.revision;
}

void TrackerService::failPendingTransmission(bool timeout) {
    const bool failedManual = pendingManual_;
    pendingTx_ = false;
    pendingManual_ = false;
    pendingTxSequence_ = 0;
    pendingReason_ = BeaconReason::None;
    view_.txPending = false;
    retryNotBeforeMs_ = millis() + AppConfig::TRACKER_TX_RETRY_DELAY_MS;

    if (failedManual) {
        failManualBeaconLocalized(
            timeout
                ? "BOOT beacon selhal: chybi potvrzeni TX"
                : "BOOT beacon selhal pri RF vysilani",
            timeout
                ? "BOOT beacon failed: TX confirmation timed out"
                : "BOOT beacon failed during RF transmission");
    } else {
        setStatusLocalized(
            timeout
                ? "Tracker: chybi potvrzeni TX; opakuji pozdeji"
                : "Tracker: RF vysilani selhalo; opakuji pozdeji",
            timeout
                ? "Tracker: TX confirmation timed out; retrying later"
                : "Tracker: RF transmission failed; retrying later");
    }
    ++view_.revision;
}

const char* TrackerService::reasonText(BeaconReason reason) const {
    const bool english = language_ == App::UiLanguage::English;
    switch (reason) {
        case BeaconReason::Initial: return english ? "Initial" : "Start trackeru";
        case BeaconReason::FixedInterval: return english ? "Fixed interval" : "Pevny interval";
        case BeaconReason::SpeedInterval: return english ? "Speed interval" : "Interval dle rychlosti";
        case BeaconReason::Corner: return english ? "Corner" : "Zmena smeru";
        case BeaconReason::StartMoving: return english ? "Start moving" : "Rozjezd";
        case BeaconReason::Stopped: return english ? "Stopped" : "Zastaveni";
        case BeaconReason::Manual: return english ? "Manual" : "Rucni";
        case BeaconReason::None:
        default: return "--";
    }
}

void TrackerService::setStatus(const char* text) {
    if (text == nullptr) {
        text = "";
    }
    if (std::strncmp(view_.statusText, text, sizeof(view_.statusText)) != 0) {
        std::snprintf(view_.statusText, sizeof(view_.statusText), "%s", text);
        ++view_.revision;
    }
}

void TrackerService::setStatusLocalized(const char* czech, const char* english) {
    setStatus(language_ == App::UiLanguage::English ? english : czech);
}

void TrackerService::failManualBeaconLocalized(
    const char* czech,
    const char* english) {

    manualBeaconPending_ = false;
    view_.manualBeaconPending = false;
    ++view_.manualBeaconFailures;
    ++view_.revision;
    const char* reason = language_ == App::UiLanguage::English ? english : czech;
    setStatus(reason);
    LOG_E("TRACKER", "%s", reason != nullptr ? reason : "Manual beacon failed");
}

}  // namespace Services
