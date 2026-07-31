#pragma once
#include <cstdint>
namespace Services { class GpsService { public: struct ViewState {
    bool utcTimeValid = false;
    bool utcDateValid = false;
    std::uint8_t utcHour = 0;
    std::uint8_t utcMinute = 0;
    std::uint8_t utcSecond = 0;
    std::uint8_t utcDay = 0;
    std::uint8_t utcMonth = 0;
    std::uint16_t utcYear = 0;
}; }; }
