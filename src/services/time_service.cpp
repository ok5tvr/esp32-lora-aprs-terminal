#include "services/time_service.h"

#include <Arduino.h>
#include <Wire.h>

#include <cstdio>
#include <cstring>

#include "app_config.h"
#include "app_log.h"
#include "board_pins.h"

namespace Services {
namespace {

constexpr std::uint8_t RTC_SECONDS_REGISTER = 0x04;
constexpr std::uint8_t RTC_CONTROL1_REGISTER = 0x00;
constexpr std::uint8_t RTC_STOP_BIT = 0x20;
constexpr std::uint32_t RTC_READ_INTERVAL_MS = 60000U;
constexpr std::uint32_t RTC_GPS_RESYNC_INTERVAL_MS = 21600000U;  // 6 h
constexpr std::uint32_t RTC_WRITE_RETRY_INTERVAL_MS = 60000U;

std::uint8_t bcdToDecimal(std::uint8_t value) {
    return static_cast<std::uint8_t>((value >> 4U) * 10U + (value & 0x0FU));
}

std::uint8_t decimalToBcd(std::uint8_t value) {
    return static_cast<std::uint8_t>(((value / 10U) << 4U) | (value % 10U));
}

bool leapYear(std::int32_t year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

std::uint8_t daysInMonth(std::int32_t year, std::uint8_t month) {
    static constexpr std::uint8_t DAYS[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    if (month < 1U || month > 12U) {
        return 0;
    }
    return month == 2U && leapYear(year)
        ? 29U
        : DAYS[month - 1U];
}

bool validDateTime(const TimeService::DateTime& value) {
    return value.year >= 2000U && value.year <= 2099U &&
        value.month >= 1U && value.month <= 12U &&
        value.day >= 1U && value.day <= daysInMonth(value.year, value.month) &&
        value.hour <= 23U && value.minute <= 59U && value.second <= 59U;
}

// Howard Hinnant's civil calendar conversion, adapted for embedded use.
std::int64_t daysFromCivil(std::int32_t year, std::uint8_t month, std::uint8_t day) {
    year -= month <= 2U ? 1 : 0;
    const std::int32_t era = (year >= 0 ? year : year - 399) / 400;
    const std::uint32_t yoe = static_cast<std::uint32_t>(year - era * 400);
    const std::uint32_t adjustedMonth = month > 2U ? month - 3U : month + 9U;
    const std::uint32_t doy = (153U * adjustedMonth + 2U) / 5U + day - 1U;
    const std::uint32_t doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    return static_cast<std::int64_t>(era) * 146097LL +
        static_cast<std::int64_t>(doe) - 719468LL;
}

void civilFromDays(
    std::int64_t days,
    std::uint16_t& year,
    std::uint8_t& month,
    std::uint8_t& day) {

    days += 719468LL;
    const std::int64_t era = (days >= 0 ? days : days - 146096LL) / 146097LL;
    const std::uint32_t doe = static_cast<std::uint32_t>(days - era * 146097LL);
    const std::uint32_t yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    std::int32_t y = static_cast<std::int32_t>(yoe) + static_cast<std::int32_t>(era * 400LL);
    const std::uint32_t doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    const std::uint32_t mp = (5U * doy + 2U) / 153U;
    const std::uint32_t d = doy - (153U * mp + 2U) / 5U + 1U;
    const std::uint32_t m = mp < 10U ? mp + 3U : mp - 9U;
    y += m <= 2U ? 1 : 0;
    year = static_cast<std::uint16_t>(y);
    month = static_cast<std::uint8_t>(m);
    day = static_cast<std::uint8_t>(d);
}

std::int64_t epochFromDateTime(const TimeService::DateTime& value) {
    return daysFromCivil(value.year, value.month, value.day) * 86400LL +
        static_cast<std::int64_t>(value.hour) * 3600LL +
        static_cast<std::int64_t>(value.minute) * 60LL + value.second;
}

TimeService::DateTime dateTimeFromEpoch(std::int64_t epoch) {
    TimeService::DateTime value;
    std::int64_t days = epoch / 86400LL;
    std::int64_t seconds = epoch % 86400LL;
    if (seconds < 0) {
        seconds += 86400LL;
        --days;
    }
    civilFromDays(days, value.year, value.month, value.day);
    value.hour = static_cast<std::uint8_t>(seconds / 3600LL);
    seconds %= 3600LL;
    value.minute = static_cast<std::uint8_t>(seconds / 60LL);
    value.second = static_cast<std::uint8_t>(seconds % 60LL);
    return value;
}

std::uint8_t weekdayFromDate(std::uint16_t year, std::uint8_t month, std::uint8_t day) {
    // 1970-01-01 was Thursday (4 when Sunday is 0).
    std::int64_t weekday = (daysFromCivil(year, month, day) + 4LL) % 7LL;
    if (weekday < 0) {
        weekday += 7LL;
    }
    return static_cast<std::uint8_t>(weekday);
}

std::uint8_t lastSunday(std::uint16_t year, std::uint8_t month) {
    const std::uint8_t lastDay = daysInMonth(year, month);
    const std::uint8_t weekday = weekdayFromDate(year, month, lastDay);
    return static_cast<std::uint8_t>(lastDay - weekday);
}

bool centralEuropeSummerTime(std::int64_t utcEpoch) {
    const TimeService::DateTime utc = dateTimeFromEpoch(utcEpoch);
    TimeService::DateTime start;
    start.year = utc.year;
    start.month = 3;
    start.day = lastSunday(utc.year, 3);
    start.hour = 1;

    TimeService::DateTime end;
    end.year = utc.year;
    end.month = 10;
    end.day = lastSunday(utc.year, 10);
    end.hour = 1;

    return utcEpoch >= epochFromDateTime(start) && utcEpoch < epochFromDateTime(end);
}

bool readRegister(std::uint8_t address, std::uint8_t& value) {
    Wire.beginTransmission(BoardPins::RTC_ADDRESS);
    Wire.write(address);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    if (Wire.requestFrom(BoardPins::RTC_ADDRESS, static_cast<std::uint8_t>(1)) != 1) {
        return false;
    }
    value = static_cast<std::uint8_t>(Wire.read());
    return true;
}

bool writeRegister(std::uint8_t address, std::uint8_t value) {
    Wire.beginTransmission(BoardPins::RTC_ADDRESS);
    Wire.write(address);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

}  // namespace

bool TimeService::begin(std::uint32_t now) {
    view_ = ViewState{};
    view_.rtcAvailable = probeRtc();
    if (!view_.rtcAvailable) {
        LOG_E("TIME", "PCF85063 RTC is unavailable");
        return false;
    }

    if (!startRtc()) {
        LOG_E("TIME", "PCF85063 oscillator could not be started");
    }

    DateTime rtcValue;
    if (readRtc(rtcValue)) {
        view_.rtcTimeValid = true;
        setBase(now, epochFromDateTime(rtcValue), Source::Rtc);
        updateView(now, true);
        LOG_I(
            "TIME",
            "RTC ready: %04u-%02u-%02u %02u:%02u:%02u UTC",
            static_cast<unsigned>(rtcValue.year),
            static_cast<unsigned>(rtcValue.month),
            static_cast<unsigned>(rtcValue.day),
            static_cast<unsigned>(rtcValue.hour),
            static_cast<unsigned>(rtcValue.minute),
            static_cast<unsigned>(rtcValue.second));
    } else {
        LOG_I("TIME", "RTC found, waiting for GPS time synchronization");
    }
    lastRtcReadAt_ = now;
    return true;
}

void TimeService::update(std::uint32_t now, const GpsService::ViewState& gps) {
    const bool gpsValid = gps.utcDateValid && gps.utcTimeValid &&
        gps.utcYear >= 2000U && gps.utcYear <= 2099U;
    view_.gpsTimeValid = gpsValid;

    if (gpsValid) {
        DateTime gpsValue;
        gpsValue.year = gps.utcYear;
        gpsValue.month = gps.utcMonth;
        gpsValue.day = gps.utcDay;
        gpsValue.hour = gps.utcHour;
        gpsValue.minute = gps.utcMinute;
        gpsValue.second = gps.utcSecond;
        if (validDateTime(gpsValue)) {
            const std::int64_t gpsEpoch = epochFromDateTime(gpsValue);
            setBase(now, gpsEpoch, Source::Gps);
            lastGpsSyncAt_ = now;

            const std::uint32_t rtcWriteInterval = rtcEverSynchronized_
                ? RTC_GPS_RESYNC_INTERVAL_MS
                : RTC_WRITE_RETRY_INTERVAL_MS;
            const bool rtcWriteDue = view_.rtcAvailable &&
                (lastRtcWriteAt_ == 0U || now - lastRtcWriteAt_ >= rtcWriteInterval);
            if (rtcWriteDue) {
                lastRtcWriteAt_ = now;
                if (writeRtc(gpsValue)) {
                    view_.rtcTimeValid = true;
                    rtcEverSynchronized_ = true;
                    LOG_I("TIME", "RTC synchronized from GPS");
                } else {
                    LOG_E("TIME", "RTC synchronization write failed");
                }
            }
        }
    } else {
        // begin() already attempts an RTC read. If its contents are invalid,
        // retry only once per minute instead of polling I2C in every loop.
        const bool rtcReadDue = view_.rtcAvailable &&
            (gpsWasValid_ || now - lastRtcReadAt_ >= RTC_READ_INTERVAL_MS);
        if (rtcReadDue) {
            DateTime rtcValue;
            lastRtcReadAt_ = now;
            if (readRtc(rtcValue)) {
                view_.rtcTimeValid = true;
                setBase(now, epochFromDateTime(rtcValue), Source::Rtc);
            } else if (baseValid_ && view_.source == Source::Gps) {
                view_.source = Source::Holdover;
            }
        } else if (baseValid_ && view_.source == Source::Gps) {
            view_.source = view_.rtcAvailable ? Source::Rtc : Source::Holdover;
        }
    }

    gpsWasValid_ = gpsValid;
    updateView(now);
}

const TimeService::ViewState& TimeService::viewState() const {
    return view_;
}

bool TimeService::probeRtc() {
    Wire.beginTransmission(BoardPins::RTC_ADDRESS);
    return Wire.endTransmission() == 0;
}

bool TimeService::startRtc() {
    std::uint8_t control = 0;
    if (!readRegister(RTC_CONTROL1_REGISTER, control)) {
        return false;
    }
    if ((control & RTC_STOP_BIT) == 0U) {
        return true;
    }
    return writeRegister(
        RTC_CONTROL1_REGISTER,
        static_cast<std::uint8_t>(control & ~RTC_STOP_BIT));
}

bool TimeService::readRtc(DateTime& value) {
    if (!view_.rtcAvailable) {
        return false;
    }

    Wire.beginTransmission(BoardPins::RTC_ADDRESS);
    Wire.write(RTC_SECONDS_REGISTER);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    if (Wire.requestFrom(BoardPins::RTC_ADDRESS, static_cast<std::uint8_t>(7)) != 7) {
        return false;
    }

    std::uint8_t data[7] = {};
    for (std::uint8_t index = 0; index < 7U; ++index) {
        data[index] = static_cast<std::uint8_t>(Wire.read());
    }
    if ((data[0] & 0x80U) != 0U) {
        return false;
    }

    value.second = bcdToDecimal(data[0] & 0x7FU);
    value.minute = bcdToDecimal(data[1] & 0x7FU);
    value.hour = bcdToDecimal(data[2] & 0x3FU);
    value.day = bcdToDecimal(data[3] & 0x3FU);
    value.month = bcdToDecimal(data[5] & 0x1FU);
    value.year = static_cast<std::uint16_t>(2000U + bcdToDecimal(data[6]));
    return validDateTime(value);
}

bool TimeService::writeRtc(const DateTime& value) {
    if (!view_.rtcAvailable || !validDateTime(value)) {
        return false;
    }

    Wire.beginTransmission(BoardPins::RTC_ADDRESS);
    Wire.write(RTC_SECONDS_REGISTER);
    Wire.write(decimalToBcd(value.second));
    Wire.write(decimalToBcd(value.minute));
    Wire.write(decimalToBcd(value.hour));
    Wire.write(decimalToBcd(value.day));
    Wire.write(decimalToBcd(weekdayFromDate(value.year, value.month, value.day)));
    Wire.write(decimalToBcd(value.month));
    Wire.write(decimalToBcd(static_cast<std::uint8_t>(value.year % 100U)));
    return Wire.endTransmission() == 0;
}

void TimeService::setBase(std::uint32_t now, std::int64_t epoch, Source source) {
    baseEpochUtc_ = epoch;
    baseMillis_ = now;
    baseValid_ = true;
    view_.source = source;
}

void TimeService::updateView(std::uint32_t now, bool forceRevision) {
    if (!baseValid_) {
        view_.valid = false;
        if (forceRevision) {
            ++view_.revision;
        }
        return;
    }

    const std::int64_t utcEpoch = baseEpochUtc_ +
        static_cast<std::int64_t>(now - baseMillis_) / 1000LL;
    const std::uint32_t utcSecondKey = static_cast<std::uint32_t>(utcEpoch);
    if (!forceRevision && utcSecondKey == lastViewSecond_) {
        view_.lastGpsSyncAgeMs = lastGpsSyncAt_ > 0U
            ? now - lastGpsSyncAt_
            : 0xFFFFFFFFU;
        return;
    }
    lastViewSecond_ = utcSecondKey;

    const DateTime utc = dateTimeFromEpoch(utcEpoch);
    const bool summerTime = centralEuropeSummerTime(utcEpoch);
    const std::int64_t localEpoch = utcEpoch + (summerTime ? 7200LL : 3600LL);
    const DateTime local = dateTimeFromEpoch(localEpoch);

    view_.valid = true;
    view_.utcYear = utc.year;
    view_.utcMonth = utc.month;
    view_.utcDay = utc.day;
    view_.utcHour = utc.hour;
    view_.utcMinute = utc.minute;
    view_.utcSecond = utc.second;
    view_.localYear = local.year;
    view_.localMonth = local.month;
    view_.localDay = local.day;
    view_.localHour = local.hour;
    view_.localMinute = local.minute;
    view_.localSecond = local.second;
    std::snprintf(view_.timezone, sizeof(view_.timezone), "%s", summerTime ? "CEST" : "CET");
    view_.lastGpsSyncAgeMs = lastGpsSyncAt_ > 0U
        ? now - lastGpsSyncAt_
        : 0xFFFFFFFFU;
    ++view_.revision;
}

const char* timeSourceText(TimeService::Source source) {
    switch (source) {
        case TimeService::Source::Gps: return "GPS";
        case TimeService::Source::Rtc: return "RTC";
        case TimeService::Source::Holdover: return "HLD";
        case TimeService::Source::None:
        default: return "--";
    }
}

}  // namespace Services
