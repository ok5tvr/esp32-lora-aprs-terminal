#pragma once

#include <cstdint>

#include "services/gps_service.h"

namespace Services {

class TimeService {
public:
    enum class Source : std::uint8_t {
        None = 0,
        Rtc,
        Gps,
        Holdover
    };

    struct ViewState {
        bool valid = false;
        bool rtcAvailable = false;
        bool rtcTimeValid = false;
        bool gpsTimeValid = false;
        Source source = Source::None;
        std::uint16_t utcYear = 0;
        std::uint8_t utcMonth = 0;
        std::uint8_t utcDay = 0;
        std::uint8_t utcHour = 0;
        std::uint8_t utcMinute = 0;
        std::uint8_t utcSecond = 0;
        std::uint16_t localYear = 0;
        std::uint8_t localMonth = 0;
        std::uint8_t localDay = 0;
        std::uint8_t localHour = 0;
        std::uint8_t localMinute = 0;
        std::uint8_t localSecond = 0;
        char timezone[6] = "--";
        std::uint32_t lastGpsSyncAgeMs = 0xFFFFFFFFU;
        std::uint32_t revision = 0;
    };

    struct DateTime {
        std::uint16_t year = 0;
        std::uint8_t month = 0;
        std::uint8_t day = 0;
        std::uint8_t hour = 0;
        std::uint8_t minute = 0;
        std::uint8_t second = 0;
    };

    bool begin(std::uint32_t now);
    void update(std::uint32_t now, const GpsService::ViewState& gps);
    const ViewState& viewState() const;

private:
    bool probeRtc();
    bool startRtc();
    bool readRtc(DateTime& value);
    bool writeRtc(const DateTime& value);
    void setBase(std::uint32_t now, std::int64_t epoch, Source source);
    void updateView(std::uint32_t now, bool forceRevision = false);

    ViewState view_;
    bool baseValid_ = false;
    std::int64_t baseEpochUtc_ = 0;
    std::uint32_t baseMillis_ = 0;
    std::uint32_t lastViewSecond_ = 0xFFFFFFFFU;
    std::uint32_t lastRtcReadAt_ = 0;
    std::uint32_t lastRtcWriteAt_ = 0;
    std::uint32_t lastGpsSyncAt_ = 0;
    bool gpsWasValid_ = false;
    bool rtcEverSynchronized_ = false;
};

const char* timeSourceText(TimeService::Source source);

}  // namespace Services
