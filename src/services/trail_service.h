#pragma once

#include <FS.h>
#include <cstddef>
#include <cstdint>

#include "services/gps_service.h"

namespace Services {

class TrailService {
public:
    static constexpr std::size_t MAX_LOGS = 8;
    static constexpr std::size_t LOG_NAME_CAPACITY = 48;

    enum class State : std::uint8_t {
        Disabled,
        WaitingForSd,
        WaitingForGps,
        Recording,
        AutoPaused,
        ManualPaused,
        Error
    };

    struct LogEntry {
        char name[LOG_NAME_CAPACITY] = {};
        std::uint32_t sizeBytes = 0;
    };

    struct ViewState {
        bool configuredEnabled = false;
        bool sdMounted = false;
        bool fileOpen = false;
        bool manualPaused = false;
        bool autoPaused = false;
        State state = State::Disabled;
        char statusText[96] = "Stopar je vypnuty.";
        char activeFile[LOG_NAME_CAPACITY] = {};
        std::uint32_t pointsWritten = 0;
        std::uint32_t droppedLines = 0;
        std::uint32_t elapsedSeconds = 0;
        double distanceKm = 0.0;
        LogEntry logs[MAX_LOGS] = {};
        std::uint8_t logCount = 0;
        std::uint32_t revision = 0;
    };

    bool begin();
    void update(
        std::uint32_t now,
        bool configuredEnabled,
        const GpsService::ViewState& gps);
    bool toggleManualPause(
        std::uint32_t now,
        const GpsService::ViewState& gps,
        char* errorText,
        std::size_t errorTextCapacity);
    void refreshLogs();
    const ViewState& viewState() const;

private:
    static constexpr std::size_t LINE_CAPACITY = 192;
    static constexpr std::size_t QUEUE_CAPACITY = 12;

    bool openSession(const GpsService::ViewState& gps, std::uint32_t now);
    void closeSession(std::uint32_t now, const GpsService::ViewState* gps);
    void setState(State state, const char* text);
    void setError(const char* text);
    void resetSessionRuntime();
    bool queueLine(const char* line);
    void queueEvent(
        const char* eventName,
        const GpsService::ViewState& gps);
    void queuePoint(
        const GpsService::ViewState& gps,
        const char* stateText,
        std::uint32_t now);
    void updateElapsed(std::uint32_t now);
    void serviceStorage(std::uint32_t now);
    void updateActiveLogSize();
    bool movementDetected(const GpsService::ViewState& gps) const;
    bool shouldRecordPoint(
        const GpsService::ViewState& gps,
        std::uint32_t now,
        bool force) const;
    static void formatUtc(
        const GpsService::ViewState& gps,
        char* output,
        std::size_t outputCapacity);
    static const char* baseName(const char* path);
    static bool isTxtFile(const char* name);
    static void copyError(
        char* output,
        std::size_t outputCapacity,
        const char* text);

    ViewState view_;
    File file_;
    char activePath_[80] = {};
    char queue_[QUEUE_CAPACITY][LINE_CAPACITY] = {};
    std::uint8_t queueHead_ = 0;
    std::uint8_t queueTail_ = 0;
    std::uint8_t queueCount_ = 0;
    std::uint32_t sessionStartedAt_ = 0;
    std::uint32_t lastPointAt_ = 0;
    std::uint32_t lastFlushAt_ = 0;
    std::uint32_t stationarySince_ = 0;
    std::uint32_t linesSinceFlush_ = 0;
    bool haveLastPoint_ = false;
    double lastPointLatitude_ = 0.0;
    double lastPointLongitude_ = 0.0;
    bool haveMovementReference_ = false;
    double movementReferenceLatitude_ = 0.0;
    double movementReferenceLongitude_ = 0.0;
    bool fatalError_ = false;
};

}  // namespace Services
