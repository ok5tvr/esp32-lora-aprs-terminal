#include "services/tracker_service.h"

#include <Arduino.h>
#include <aprs_codec.h>
#include <cmath>
#include <cstdio>
#include <cstring>

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

}  // namespace

void TrackerService::begin() {
    view_ = ViewState{};
    lastSettingsRevision_ = 0xFFFFFFFFU;
    armedAtMs_ = 0;
    manualRequestedAtMs_ = 0;
    manualBeaconPending_ = false;
    hasTransmitted_ = false;
    lastCourseValid_ = false;
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
    if (lastSettingsRevision_ != settings.revision) {
        lastSettingsRevision_ = settings.revision;
        view_.configuredEnabled = settings.trackerEnabled;
        view_.smartBeacon = settings.trackerMode == App::TrackerBeaconMode::SmartBeacon;

        // Re-arm scheduled tracking after every saved configuration change.
        // A pending physical-button request is intentionally preserved.
        hasTransmitted_ = false;
        lastCourseValid_ = false;
        view_.nextTransmitSeconds = 0;
        if (settings.trackerEnabled) {
            armedAtMs_ = now + AppConfig::TRACKER_START_DELAY_MS;
        }
        ++view_.revision;
    }

    view_.configuredEnabled = settings.trackerEnabled;
    view_.smartBeacon = settings.trackerMode == App::TrackerBeaconMode::SmartBeacon;
    view_.usingGps = settings.trackerSource == App::TrackerPositionSource::Gps;
    view_.manualBeaconPending = manualBeaconPending_;
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

    // A short BOOT-button click requests one position packet even when the
    // periodic tracker is disabled. The selected source, format and symbol are
    // still taken from the saved tracker settings.
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
        if (buildAndSend(now, settings, gps, radio)) {
            manualBeaconPending_ = false;
            view_.manualBeaconPending = false;
            ++view_.manualPacketsSent;
            ++view_.revision;
            if (settings.trackerEnabled) {
                setStatusLocalized(
                    "BOOT beacon zarazen; plan trackeru restartovan",
                    "BOOT beacon queued; tracker schedule restarted");
            } else {
                setStatusLocalized(
                    "BOOT beacon zarazen; tracker zustava vypnut",
                    "BOOT beacon queued; tracker remains off");
            }
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

    std::uint32_t intervalSeconds = settings.trackerFixedIntervalSeconds;
    bool due = false;
    if (!hasTransmitted_) {
        due = static_cast<std::int32_t>(now - armedAtMs_) >= 0;
        view_.nextTransmitSeconds = due
            ? 0
            : static_cast<std::uint32_t>((armedAtMs_ - now + 999U) / 1000U);
    } else {
        if (settings.trackerMode == App::TrackerBeaconMode::SmartBeacon) {
            intervalSeconds = intervalForSpeed(speedValid ? view_.speedKmh : 0.0F);
            due = cornerBeaconDue(
                now,
                speedValid ? view_.speedKmh : 0.0F,
                view_.courseDegrees,
                courseValid);
        }
        const std::uint32_t elapsedMs = now - view_.lastTransmitAtMs;
        const std::uint32_t intervalMs = intervalSeconds * 1000U;
        if (elapsedMs >= intervalMs) {
            due = true;
        }
        view_.nextTransmitSeconds = due || elapsedMs >= intervalMs
            ? 0
            : static_cast<std::uint32_t>((intervalMs - elapsedMs + 999U) / 1000U);
    }

    if (due) {
        if (buildAndSend(now, settings, gps, radio)) {
            if (settings.trackerMode == App::TrackerBeaconMode::SmartBeacon) {
                setStatusLocalized(
                    "Tracker aktivni: SmartBeacon paket zarazen",
                    "Tracker active: SmartBeacon packet queued");
            } else {
                setStatusLocalized(
                    "Tracker aktivni: casovy paket zarazen",
                    "Tracker active: scheduled packet queued");
            }
        } else {
            setStatusLocalized(
                "Tracker: odeslani se nepodarilo",
                "Tracker: transmission failed");
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

std::uint32_t TrackerService::intervalForSpeed(float speedKmh) const {
    if (!std::isfinite(speedKmh) || speedKmh <= AppConfig::SMARTBEACON_LOW_SPEED_KMH) {
        return AppConfig::SMARTBEACON_SLOW_RATE_SECONDS;
    }
    if (speedKmh >= AppConfig::SMARTBEACON_HIGH_SPEED_KMH) {
        return AppConfig::SMARTBEACON_FAST_RATE_SECONDS;
    }

    const float calculated =
        static_cast<float>(AppConfig::SMARTBEACON_FAST_RATE_SECONDS) *
        AppConfig::SMARTBEACON_HIGH_SPEED_KMH / speedKmh;
    std::uint32_t interval = static_cast<std::uint32_t>(calculated + 0.5F);
    if (interval < AppConfig::SMARTBEACON_FAST_RATE_SECONDS) {
        interval = AppConfig::SMARTBEACON_FAST_RATE_SECONDS;
    }
    if (interval > AppConfig::SMARTBEACON_SLOW_RATE_SECONDS) {
        interval = AppConfig::SMARTBEACON_SLOW_RATE_SECONDS;
    }
    return interval;
}

bool TrackerService::cornerBeaconDue(
    std::uint32_t now,
    float speedKmh,
    float courseDegrees,
    bool courseValid) const {

    if (!hasTransmitted_ || !lastCourseValid_ || !courseValid ||
        speedKmh <= AppConfig::SMARTBEACON_LOW_SPEED_KMH) {
        return false;
    }
    if (now - view_.lastTransmitAtMs < AppConfig::SMARTBEACON_MIN_TURN_SECONDS * 1000U) {
        return false;
    }

    const float threshold = AppConfig::SMARTBEACON_TURN_ANGLE_DEGREES +
        AppConfig::SMARTBEACON_TURN_SLOPE / (speedKmh > 1.0F ? speedKmh : 1.0F);
    return angularDifference(courseDegrees, lastCourseAtTransmit_) >= threshold;
}

bool TrackerService::buildAndSend(
    std::uint32_t now,
    const SettingsService::ViewState& settings,
    const GpsService::ViewState& gps,
    RadioService& radio) {

    const bool usingGps = settings.trackerSource == App::TrackerPositionSource::Gps;
    const bool includeCourseSpeed = usingGps && gps.speedValid && gps.courseValid;
    const double speedKnots = includeCourseSpeed
        ? static_cast<double>(gps.speedKmh) / 1.852
        : 0.0;

    const App::TrackerSymbolDefinition& symbol =
        App::trackerSymbolDefinition(settings.trackerSymbol);

    char frame[192] = {};
    if (!Aprs::buildPositionTnc2(
            settings.callsign,
            AppConfig::APRS_DESTINATION,
            view_.latitude,
            view_.longitude,
            symbol.table,
            symbol.code,
            settings.trackerFormat == App::TrackerPositionFormat::Compressed,
            includeCourseSpeed,
            view_.courseDegrees,
            speedKnots,
            AppConfig::TRACKER_COMMENT,
            frame,
            sizeof(frame))) {
        LOG_E("TRACKER", "Position frame encoding failed");
        return false;
    }

    if (!radio.queueTrackerPacket(frame, manualBeaconPending_, now)) {
        return false;
    }

    hasTransmitted_ = true;
    view_.lastTransmitAtMs = now;
    ++view_.trackerPacketsSent;
    std::snprintf(view_.lastFrame, sizeof(view_.lastFrame), "%s", frame);
    lastCourseAtTransmit_ = view_.courseDegrees;
    lastCourseValid_ = includeCourseSpeed;
    ++view_.revision;
    LOG_I("TRACKER", "TX %s", frame);
    return true;
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
