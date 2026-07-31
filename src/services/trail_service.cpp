#include "services/trail_service.h"

#include <Arduino.h>
#include <SD.h>
#include <cstdio>
#include <cstring>

#include "app_config.h"
#include "app/localization.h"
#include "app_log.h"
#include "drivers/sd_card_driver.h"
#include "services/geo_utils.h"

namespace Services {
namespace {

constexpr char LOG_DIRECTORY[] = "/STOPAR";
constexpr char HEADER_LINE_2[] =
    "# UTC;latitude;longitude;altitude_m;speed_kmh;course_deg;satellites;hdop;state\n";

}  // namespace

bool TrailService::begin() {
    view_ = ViewState{};
    localizationRevision_ = App::Localization::revision();
    view_.sdMounted = Drivers::SdCard::status().mounted;
    resetSessionRuntime();
    if (view_.sdMounted) {
        if (!SD.exists(LOG_DIRECTORY) && !SD.mkdir(LOG_DIRECTORY)) {
            setError(App::Localization::text("Nelze vytvorit slozku /STOPAR na SD karte.", "Cannot create /STOPAR folder on the SD card."));
            return false;
        }
        refreshLogs();
    } else {
        setState(State::WaitingForSd, App::Localization::text("SD karta neni dostupna.", "SD card is unavailable."));
    }
    return view_.sdMounted;
}

void TrailService::update(
    std::uint32_t now,
    bool configuredEnabled,
    const GpsService::ViewState& gps) {

    const std::uint32_t currentLocalizationRevision = App::Localization::revision();
    if (localizationRevision_ != currentLocalizationRevision) {
        localizationRevision_ = currentLocalizationRevision;
        if (fatalError_) {
            setState(
                State::Error,
                App::Localization::text("Chyba Stopare.", "Trail logger error."));
        }
    }

    const bool sdMounted = Drivers::SdCard::status().mounted;
    if (view_.sdMounted != sdMounted) {
        view_.sdMounted = sdMounted;
        ++view_.revision;
    }

    if (view_.configuredEnabled != configuredEnabled) {
        view_.configuredEnabled = configuredEnabled;
        ++view_.revision;
        if (!configuredEnabled) {
            closeSession(now, &gps);
            view_.manualPaused = false;
            view_.autoPaused = false;
            setState(State::Disabled, App::Localization::text("Stopar je vypnuty.", "Trail logger is disabled."));
            return;
        }
        fatalError_ = false;
        resetSessionRuntime();
    }

    if (!configuredEnabled) {
        if (view_.state != State::Disabled) {
            setState(State::Disabled, App::Localization::text("Stopar je vypnuty.", "Trail logger is disabled."));
        }
        serviceStorage(now);
        return;
    }

    if (!sdMounted) {
        closeSession(now, &gps);
        setState(State::WaitingForSd, App::Localization::text("Stopar ceka na SD kartu.", "Trail logger is waiting for the SD card."));
        return;
    }

    if (fatalError_) {
        serviceStorage(now);
        return;
    }

    if (view_.manualPaused) {
        setState(State::ManualPaused, App::Localization::text("Zaznam je rucne pozastaven.", "Recording is paused manually."));
        serviceStorage(now);
        updateElapsed(now);
        return;
    }

    if (!gps.hasFix || !gps.utcDateValid || !gps.utcTimeValid) {
        setState(
            State::WaitingForGps,
            gps.hasFix ? App::Localization::text("Stopar ceka na platny GPS cas.", "Trail logger is waiting for valid GPS time.") : App::Localization::text("Stopar ceka na GPS fix.", "Trail logger is waiting for a GPS fix."));
        serviceStorage(now);
        updateElapsed(now);
        return;
    }

    if (!view_.fileOpen && !openSession(gps, now)) {
        serviceStorage(now);
        return;
    }

    const bool moving = movementDetected(gps);
    if (!haveMovementReference_) {
        haveMovementReference_ = true;
        movementReferenceLatitude_ = gps.latitude;
        movementReferenceLongitude_ = gps.longitude;
    }

    if (view_.autoPaused) {
        if (moving) {
            view_.autoPaused = false;
            stationarySince_ = 0;
            movementReferenceLatitude_ = gps.latitude;
            movementReferenceLongitude_ = gps.longitude;
            queueEvent("AUTO_RESUME", gps);
            queuePoint(gps, "RECORDING", now);
            setState(State::Recording, App::Localization::text("Pohyb obnoven, zaznam pokracuje.", "Movement resumed; recording continues."));
        } else {
            setState(State::AutoPaused, App::Localization::text("Autopauza: zarizeni se nepohybuje.", "Auto-pause: the device is stationary."));
        }
    } else {
        if (moving) {
            stationarySince_ = 0;
            movementReferenceLatitude_ = gps.latitude;
            movementReferenceLongitude_ = gps.longitude;
        } else if (stationarySince_ == 0U) {
            stationarySince_ = now;
        } else if (now - stationarySince_ >= AppConfig::TRAIL_AUTOPAUSE_DELAY_MS) {
            view_.autoPaused = true;
            queueEvent("AUTO_PAUSE", gps);
            setState(State::AutoPaused, App::Localization::text("Autopauza: zarizeni se nepohybuje.", "Auto-pause: the device is stationary."));
        }

        if (!view_.autoPaused && shouldRecordPoint(gps, now, false)) {
            queuePoint(gps, "RECORDING", now);
        }
        if (!view_.autoPaused) {
            setState(State::Recording, App::Localization::text("Trasa se zaznamenava na SD kartu.", "Track is being recorded to the SD card."));
        }
    }

    updateElapsed(now);
    serviceStorage(now);
}

bool TrailService::toggleManualPause(
    std::uint32_t now,
    const GpsService::ViewState& gps,
    char* errorText,
    std::size_t errorTextCapacity) {

    if (!view_.configuredEnabled) {
        copyError(errorText, errorTextCapacity, App::Localization::text("Nejprve zapnete Stopare na strance Tracker.", "Enable the trail logger on the Tracker page first."));
        return false;
    }
    if (!view_.sdMounted) {
        copyError(errorText, errorTextCapacity, App::Localization::text("SD karta neni dostupna.", "SD card is unavailable."));
        return false;
    }
    if (fatalError_) {
        copyError(errorText, errorTextCapacity, view_.statusText);
        return false;
    }

    view_.manualPaused = !view_.manualPaused;
    if (view_.manualPaused) {
        if (view_.fileOpen && gps.utcDateValid && gps.utcTimeValid) {
            queueEvent("MANUAL_PAUSE", gps);
        }
        setState(State::ManualPaused, App::Localization::text("Zaznam je rucne pozastaven.", "Recording is paused manually."));
        copyError(errorText, errorTextCapacity, App::Localization::text("Zaznam byl rucne pozastaven.", "Recording was paused manually."));
    } else {
        view_.autoPaused = false;
        stationarySince_ = 0;
        if (view_.fileOpen && gps.utcDateValid && gps.utcTimeValid) {
            queueEvent("MANUAL_RESUME", gps);
            if (gps.hasFix) {
                queuePoint(gps, "RECORDING", now);
            }
        }
        setState(
            gps.hasFix ? State::Recording : State::WaitingForGps,
            gps.hasFix ? App::Localization::text("Rucni pauza ukoncena, zaznam pokracuje.", "Manual pause ended; recording continues.")
                       : App::Localization::text("Rucni pauza ukoncena, ceka se na GPS fix.", "Manual pause ended; waiting for a GPS fix."));
        copyError(errorText, errorTextCapacity, App::Localization::text("Zaznam byl znovu spusten.", "Recording was resumed."));
    }
    ++view_.revision;
    return true;
}

void TrailService::refreshLogs() {
    if (!Drivers::SdCard::status().mounted) {
        view_.logCount = 0;
        ++view_.revision;
        return;
    }

    LogEntry found[MAX_LOGS] = {};
    std::uint8_t count = 0;
    File directory = SD.open(LOG_DIRECTORY);
    if (!directory || !directory.isDirectory()) {
        setError(App::Localization::text("Nelze otevrit slozku /STOPAR.", "Cannot open the /STOPAR folder."));
        return;
    }

    File entry = directory.openNextFile();
    while (entry) {
        const char* name = entry.name();
        if (!entry.isDirectory() && isTxtFile(name)) {
            LogEntry candidate;
            std::snprintf(candidate.name, sizeof(candidate.name), "%s", baseName(name));
            const std::uint64_t size = entry.size();
            candidate.sizeBytes = static_cast<std::uint32_t>(
                size > 0xFFFFFFFFULL ? 0xFFFFFFFFULL : size);

            std::uint8_t insertAt = count;
            for (std::uint8_t index = 0; index < count; ++index) {
                if (std::strcmp(candidate.name, found[index].name) > 0) {
                    insertAt = index;
                    break;
                }
            }
            if (insertAt < MAX_LOGS) {
                const std::uint8_t limit = count < MAX_LOGS
                    ? count
                    : static_cast<std::uint8_t>(MAX_LOGS - 1U);
                for (std::uint8_t index = limit; index > insertAt; --index) {
                    found[index] = found[index - 1U];
                }
                found[insertAt] = candidate;
                if (count < MAX_LOGS) {
                    ++count;
                }
            }
        }
        entry.close();
        entry = directory.openNextFile();
    }
    directory.close();

    view_.logCount = count;
    for (std::size_t index = 0; index < MAX_LOGS; ++index) {
        view_.logs[index] = found[index];
    }
    ++view_.revision;
}

const TrailService::ViewState& TrailService::viewState() const {
    return view_;
}

bool TrailService::openSession(
    const GpsService::ViewState& gps,
    std::uint32_t now) {

    if (!SD.exists(LOG_DIRECTORY) && !SD.mkdir(LOG_DIRECTORY)) {
        setError(App::Localization::text("Nelze vytvorit slozku /STOPAR.", "Cannot create the /STOPAR folder."));
        return false;
    }

    char stem[48] = {};
    std::snprintf(
        stem,
        sizeof(stem),
        "%04u%02u%02u_%02u%02u%02u",
        static_cast<unsigned>(gps.utcYear),
        static_cast<unsigned>(gps.utcMonth),
        static_cast<unsigned>(gps.utcDay),
        static_cast<unsigned>(gps.utcHour),
        static_cast<unsigned>(gps.utcMinute),
        static_cast<unsigned>(gps.utcSecond));

    for (std::uint8_t suffix = 0; suffix < 100U; ++suffix) {
        if (suffix == 0U) {
            std::snprintf(activePath_, sizeof(activePath_), "%s/%s.txt", LOG_DIRECTORY, stem);
        } else {
            std::snprintf(
                activePath_,
                sizeof(activePath_),
                "%s/%s_%02u.txt",
                LOG_DIRECTORY,
                stem,
                static_cast<unsigned>(suffix));
        }
        if (!SD.exists(activePath_)) {
            break;
        }
        if (suffix == 99U) {
            setError(App::Localization::text("Nelze vytvorit jedinecny nazev logu.", "Cannot create a unique log filename."));
            return false;
        }
    }

    file_ = SD.open(activePath_, FILE_WRITE);
    if (!file_) {
        setError(App::Localization::text("Nelze otevrit novy TXT log na SD karte.", "Cannot open a new TXT log on the SD card."));
        return false;
    }

    const char* headerLine1 = App::Localization::text(
        "# LoRa APRS Terminal - Stopar\n",
        "# LoRa APRS Terminal - Trail logger\n");
    const std::size_t h1 = file_.print(headerLine1);
    const std::size_t h2 = file_.print(HEADER_LINE_2);
    if (h1 != std::strlen(headerLine1) || h2 != std::strlen(HEADER_LINE_2)) {
        file_.close();
        setError(App::Localization::text("Zapis hlavicky logu na SD selhal.", "Writing the log header to the SD card failed."));
        return false;
    }
    file_.flush();

    resetSessionRuntime();
    view_.fileOpen = true;
    sessionStartedAt_ = now;
    lastFlushAt_ = now;
    std::snprintf(view_.activeFile, sizeof(view_.activeFile), "%s", baseName(activePath_));
    queueEvent("START", gps);
    queuePoint(gps, "RECORDING", now);
    setState(State::Recording, App::Localization::text("Novy zaznam trasy byl spusten.", "A new track recording was started."));
    refreshLogs();
    LOG_I("TRAIL", "Recording started: %s", activePath_);
    return true;
}

void TrailService::closeSession(
    std::uint32_t now,
    const GpsService::ViewState* gps) {

    if (!view_.fileOpen) {
        resetSessionRuntime(false);
        return;
    }

    if (gps != nullptr && gps->utcDateValid && gps->utcTimeValid) {
        queueEvent("STOP", *gps);
    }
    while (queueCount_ > 0U && file_) {
        serviceStorage(now);
    }
    if (file_) {
        file_.flush();
        file_.close();
    }
    view_.fileOpen = false;
    view_.activeFile[0] = '\0';
    refreshLogs();
    LOG_I("TRAIL", "Recording stopped");
    resetSessionRuntime(false);
}

void TrailService::setState(State state, const char* text) {
    const bool changed = view_.state != state ||
        std::strncmp(view_.statusText, text != nullptr ? text : "", sizeof(view_.statusText)) != 0;
    view_.state = state;
    std::snprintf(view_.statusText, sizeof(view_.statusText), "%s", text != nullptr ? text : "");
    if (changed) {
        ++view_.revision;
    }
}

void TrailService::setError(const char* text) {
    fatalError_ = true;
    setState(State::Error, text != nullptr ? text : App::Localization::text("Chyba Stopare.", "Trail logger error."));
    LOG_E("TRAIL", "%s", view_.statusText);
}

void TrailService::resetSessionRuntime(bool clearRecentTrail) {
    queueHead_ = 0;
    queueTail_ = 0;
    queueCount_ = 0;
    sessionStartedAt_ = 0;
    lastPointAt_ = 0;
    lastFlushAt_ = 0;
    stationarySince_ = 0;
    linesSinceFlush_ = 0;
    haveLastPoint_ = false;
    haveMovementReference_ = false;
    view_.pointsWritten = 0;
    view_.droppedLines = 0;
    view_.elapsedSeconds = 0;
    view_.distanceKm = 0.0;
    view_.autoPaused = false;
    if (clearRecentTrail) {
        view_.recentPointCount = 0;
        ++view_.recentPointRevision;
    }
}

bool TrailService::queueLine(const char* line) {
    if (line == nullptr || line[0] == '\0') {
        return false;
    }
    if (queueCount_ >= QUEUE_CAPACITY) {
        ++view_.droppedLines;
        ++view_.revision;
        LOG_E("TRAIL", "Write queue full, line dropped");
        return false;
    }
    std::snprintf(queue_[queueTail_], LINE_CAPACITY, "%s", line);
    queueTail_ = static_cast<std::uint8_t>((queueTail_ + 1U) % QUEUE_CAPACITY);
    ++queueCount_;
    return true;
}

void TrailService::queueEvent(
    const char* eventName,
    const GpsService::ViewState& gps) {

    char utc[32] = {};
    formatUtc(gps, utc, sizeof(utc));
    char line[LINE_CAPACITY] = {};
    std::snprintf(
        line,
        sizeof(line),
        "# EVENT;%s;%s\n",
        utc,
        eventName != nullptr ? eventName : "UNKNOWN");
    queueLine(line);
}

void TrailService::queuePoint(
    const GpsService::ViewState& gps,
    const char* stateText,
    std::uint32_t now) {

    char utc[32] = {};
    formatUtc(gps, utc, sizeof(utc));
    char line[LINE_CAPACITY] = {};
    std::snprintf(
        line,
        sizeof(line),
        "%s;%.6f;%.6f;%.1f;%.1f;%.1f;%u;%.1f;%s\n",
        utc,
        gps.latitude,
        gps.longitude,
        gps.altitudeMeters,
        static_cast<double>(gps.speedValid ? gps.speedKmh : 0.0F),
        static_cast<double>(gps.courseValid ? gps.courseDegrees : 0.0F),
        static_cast<unsigned>(gps.satellites),
        static_cast<double>(gps.hdop),
        stateText != nullptr ? stateText : "RECORDING");

    if (!queueLine(line)) {
        return;
    }

    if (haveLastPoint_) {
        const DistanceBearing movement = calculateDistanceBearing(
            lastPointLatitude_,
            lastPointLongitude_,
            gps.latitude,
            gps.longitude);
        if (movement.valid && movement.distanceKm <= AppConfig::TRAIL_MAX_POINT_JUMP_KM) {
            view_.distanceKm += movement.distanceKm;
        }
    }
    haveLastPoint_ = true;
    lastPointLatitude_ = gps.latitude;
    lastPointLongitude_ = gps.longitude;
    lastPointAt_ = now;

    if (view_.recentPointCount < AppConfig::MAP_RECENT_TRAIL_POINTS) {
        view_.recentPoints[view_.recentPointCount++] = {gps.latitude, gps.longitude};
    } else {
        std::memmove(
            &view_.recentPoints[0],
            &view_.recentPoints[1],
            (AppConfig::MAP_RECENT_TRAIL_POINTS - 1U) * sizeof(view_.recentPoints[0]));
        view_.recentPoints[AppConfig::MAP_RECENT_TRAIL_POINTS - 1U] = {
            gps.latitude, gps.longitude};
    }
    ++view_.recentPointRevision;
    ++view_.pointsWritten;
    ++view_.revision;
}

void TrailService::updateElapsed(std::uint32_t now) {
    if (sessionStartedAt_ == 0U) {
        return;
    }
    const std::uint32_t elapsed = (now - sessionStartedAt_) / 1000U;
    if (elapsed != view_.elapsedSeconds) {
        view_.elapsedSeconds = elapsed;
        ++view_.revision;
    }
}

void TrailService::serviceStorage(std::uint32_t now) {
    if (!view_.fileOpen || !file_) {
        return;
    }

    if (queueCount_ > 0U) {
        const char* line = queue_[queueHead_];
        const std::size_t length = std::strlen(line);
        const std::size_t written = file_.write(
            reinterpret_cast<const std::uint8_t*>(line),
            length);
        if (written != length) {
            setError(App::Localization::text("Zapis trasy na SD kartu selhal.", "Writing the track to the SD card failed."));
            file_.close();
            view_.fileOpen = false;
            return;
        }
        queueHead_ = static_cast<std::uint8_t>((queueHead_ + 1U) % QUEUE_CAPACITY);
        --queueCount_;
        ++linesSinceFlush_;
        updateActiveLogSize();
    }

    if ((linesSinceFlush_ >= AppConfig::TRAIL_FLUSH_AFTER_LINES ||
         now - lastFlushAt_ >= AppConfig::TRAIL_FLUSH_INTERVAL_MS) &&
        queueCount_ == 0U) {
        file_.flush();
        linesSinceFlush_ = 0;
        lastFlushAt_ = now;
    }
}

void TrailService::updateActiveLogSize() {
    if (!view_.fileOpen || !file_ || view_.activeFile[0] == '\0') {
        return;
    }
    const std::uint64_t size = file_.size();
    const std::uint32_t boundedSize = static_cast<std::uint32_t>(
        size > 0xFFFFFFFFULL ? 0xFFFFFFFFULL : size);
    for (std::uint8_t index = 0; index < view_.logCount; ++index) {
        if (std::strcmp(view_.logs[index].name, view_.activeFile) == 0) {
            if (view_.logs[index].sizeBytes != boundedSize) {
                view_.logs[index].sizeBytes = boundedSize;
                ++view_.revision;
            }
            return;
        }
    }
}

bool TrailService::movementDetected(const GpsService::ViewState& gps) const {
    if (gps.speedValid && gps.speedKmh >= AppConfig::TRAIL_RESUME_SPEED_KMH) {
        return true;
    }
    if (!haveMovementReference_) {
        return true;
    }
    const DistanceBearing movement = calculateDistanceBearing(
        movementReferenceLatitude_,
        movementReferenceLongitude_,
        gps.latitude,
        gps.longitude);
    return movement.valid &&
        movement.distanceKm * 1000.0 >= AppConfig::TRAIL_RESUME_DISTANCE_METERS;
}

bool TrailService::shouldRecordPoint(
    const GpsService::ViewState& gps,
    std::uint32_t now,
    bool force) const {

    if (force || !haveLastPoint_) {
        return true;
    }
    const std::uint32_t elapsed = now - lastPointAt_;
    if (elapsed < AppConfig::TRAIL_SAMPLE_INTERVAL_MS) {
        return false;
    }
    const DistanceBearing movement = calculateDistanceBearing(
        lastPointLatitude_,
        lastPointLongitude_,
        gps.latitude,
        gps.longitude);
    const bool movedEnough = movement.valid &&
        movement.distanceKm * 1000.0 >= AppConfig::TRAIL_MIN_POINT_DISTANCE_METERS;
    return movedEnough || elapsed >= AppConfig::TRAIL_MAX_POINT_INTERVAL_MS;
}

void TrailService::formatUtc(
    const GpsService::ViewState& gps,
    char* output,
    std::size_t outputCapacity) {

    if (output == nullptr || outputCapacity == 0) {
        return;
    }
    if (!gps.utcDateValid || !gps.utcTimeValid) {
        std::snprintf(output, outputCapacity, "0000-00-00T00:00:00Z");
        return;
    }
    std::snprintf(
        output,
        outputCapacity,
        "%04u-%02u-%02uT%02u:%02u:%02uZ",
        static_cast<unsigned>(gps.utcYear),
        static_cast<unsigned>(gps.utcMonth),
        static_cast<unsigned>(gps.utcDay),
        static_cast<unsigned>(gps.utcHour),
        static_cast<unsigned>(gps.utcMinute),
        static_cast<unsigned>(gps.utcSecond));
}

const char* TrailService::baseName(const char* path) {
    if (path == nullptr) {
        return "";
    }
    const char* slash = std::strrchr(path, '/');
    return slash != nullptr ? slash + 1 : path;
}

bool TrailService::isTxtFile(const char* name) {
    if (name == nullptr) {
        return false;
    }
    const char* extension = std::strrchr(name, '.');
    if (extension == nullptr) {
        return false;
    }
    return std::strcmp(extension, ".txt") == 0 ||
        std::strcmp(extension, ".TXT") == 0;
}

void TrailService::copyError(
    char* output,
    std::size_t outputCapacity,
    const char* text) {

    if (output == nullptr || outputCapacity == 0) {
        return;
    }
    std::snprintf(output, outputCapacity, "%s", text != nullptr ? text : "");
}

}  // namespace Services
