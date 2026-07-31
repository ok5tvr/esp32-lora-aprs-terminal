#pragma once

#include <cstdint>

#include "services/geo_utils.h"

namespace Services {

class AstronomyService {
public:
    enum class MoonPhase : std::uint8_t {
        NewMoon = 0,
        WaxingCrescent,
        FirstQuarter,
        WaxingGibbous,
        FullMoon,
        WaningGibbous,
        LastQuarter,
        WaningCrescent
    };

    struct EventTime {
        bool valid = false;
        std::uint8_t hour = 0;
        std::uint8_t minute = 0;
    };

    struct ViewState {
        bool valid = false;
        bool timeValid = false;
        bool positionValid = false;
        bool positionFromGps = false;
        std::uint16_t year = 0;
        std::uint8_t month = 0;
        std::uint8_t day = 0;
        double latitude = 0.0;
        double longitude = 0.0;
        EventTime sunrise;
        EventTime sunset;
        EventTime moonrise;
        EventTime moonset;
        bool sunAboveAllDay = false;
        bool sunBelowAllDay = false;
        bool moonAboveAllDay = false;
        bool moonBelowAllDay = false;
        std::uint16_t daylightMinutes = 0;
        MoonPhase moonPhase = MoonPhase::NewMoon;
        std::uint8_t moonIlluminationPercent = 0;
        float moonAgeDays = 0.0F;
        float moonElongationDegrees = 0.0F;
        float sunAltitudeDegrees = 0.0F;
        std::uint32_t revision = 0;
    };

    void update(
        std::uint32_t now,
        bool timeValid,
        std::uint16_t localYear,
        std::uint8_t localMonth,
        std::uint8_t localDay,
        std::uint16_t utcYear,
        std::uint8_t utcMonth,
        std::uint8_t utcDay,
        std::uint8_t utcHour,
        std::uint8_t utcMinute,
        std::uint8_t utcSecond,
        const PositionReference& reference);
    const ViewState& viewState() const;

private:
    ViewState view_;
    bool observedPositionValid_ = false;
    bool observedPositionFromGps_ = false;
    double observedLatitude_ = 0.0;
    double observedLongitude_ = 0.0;
    std::uint16_t observedYear_ = 0;
    std::uint8_t observedMonth_ = 0;
    std::uint8_t observedDay_ = 0;
    bool dynamicUpdateInitialized_ = false;
    std::uint32_t lastDynamicUpdateAt_ = 0;
};

const char* moonPhaseTextCzech(AstronomyService::MoonPhase phase);
const char* moonPhaseTextEnglish(AstronomyService::MoonPhase phase);

}  // namespace Services
