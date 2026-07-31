#include "services/astronomy_service.h"

#include <algorithm>
#include <cmath>

namespace Services {
namespace {

constexpr double PI = 3.14159265358979323846;
constexpr double DEG_TO_RAD = PI / 180.0;
constexpr double RAD_TO_DEG = 180.0 / PI;
constexpr double UNIX_EPOCH_JULIAN_DAY = 2440587.5;
constexpr double J2000_JULIAN_DAY = 2451545.0;
constexpr double SCHLYTER_EPOCH_JULIAN_DAY = 2451543.5;
constexpr double SYNODIC_MONTH_DAYS = 29.530588853;
constexpr std::int64_t SECONDS_PER_DAY = 86400LL;
constexpr std::int64_t SEARCH_MARGIN_SECONDS = 3LL * 3600LL;
constexpr std::int64_t SEARCH_STEP_SECONDS = 3600LL;
constexpr double POSITION_RECALCULATION_DISTANCE_KM = 5.0;
constexpr std::uint32_t DYNAMIC_UPDATE_INTERVAL_MS = 300000UL;

struct EquatorialPosition {
    double rightAscensionDegrees = 0.0;
    double declinationDegrees = 0.0;
    double eclipticLongitudeDegrees = 0.0;
    double distanceEarthRadii = 0.0;
};

struct CrossingSummary {
    bool riseValid = false;
    bool setValid = false;
    std::int64_t riseUtcEpoch = 0;
    std::int64_t setUtcEpoch = 0;
    bool aboveAllDay = false;
    bool belowAllDay = false;
};

double normalizeDegrees(double value) {
    value = std::fmod(value, 360.0);
    if (value < 0.0) {
        value += 360.0;
    }
    return value;
}

double normalizeSignedDegrees(double value) {
    value = normalizeDegrees(value);
    return value > 180.0 ? value - 360.0 : value;
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
    return month == 2U && leapYear(year) ? 29U : DAYS[month - 1U];
}

bool validDate(std::uint16_t year, std::uint8_t month, std::uint8_t day) {
    return year >= 2000U && year <= 2099U &&
        month >= 1U && month <= 12U &&
        day >= 1U && day <= daysInMonth(year, month);
}

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

std::uint8_t weekdayFromDate(std::uint16_t year, std::uint8_t month, std::uint8_t day) {
    std::int64_t weekday = (daysFromCivil(year, month, day) + 4LL) % 7LL;
    if (weekday < 0) {
        weekday += 7LL;
    }
    return static_cast<std::uint8_t>(weekday);
}

std::uint8_t lastSunday(std::uint16_t year, std::uint8_t month) {
    const std::uint8_t lastDay = daysInMonth(year, month);
    return static_cast<std::uint8_t>(lastDay - weekdayFromDate(year, month, lastDay));
}

std::int64_t civilEpoch(
    std::uint16_t year,
    std::uint8_t month,
    std::uint8_t day,
    std::uint8_t hour = 0,
    std::uint8_t minute = 0,
    std::uint8_t second = 0) {

    return daysFromCivil(year, month, day) * SECONDS_PER_DAY +
        static_cast<std::int64_t>(hour) * 3600LL +
        static_cast<std::int64_t>(minute) * 60LL + second;
}

bool centralEuropeSummerTime(std::int64_t utcEpoch) {
    std::int64_t days = utcEpoch / SECONDS_PER_DAY;
    std::int64_t seconds = utcEpoch % SECONDS_PER_DAY;
    if (seconds < 0) {
        seconds += SECONDS_PER_DAY;
        --days;
    }
    std::uint16_t year = 0;
    std::uint8_t month = 0;
    std::uint8_t day = 0;
    civilFromDays(days, year, month, day);

    const std::int64_t start = civilEpoch(year, 3U, lastSunday(year, 3U), 1U);
    const std::int64_t end = civilEpoch(year, 10U, lastSunday(year, 10U), 1U);
    return utcEpoch >= start && utcEpoch < end;
}

std::int32_t centralEuropeOffsetSeconds(std::int64_t utcEpoch) {
    return centralEuropeSummerTime(utcEpoch) ? 7200 : 3600;
}

void localDateFromUtc(
    std::int64_t utcEpoch,
    std::uint16_t& year,
    std::uint8_t& month,
    std::uint8_t& day,
    std::uint8_t& hour,
    std::uint8_t& minute) {

    const std::int64_t localEpoch = utcEpoch + centralEuropeOffsetSeconds(utcEpoch);
    std::int64_t days = localEpoch / SECONDS_PER_DAY;
    std::int64_t seconds = localEpoch % SECONDS_PER_DAY;
    if (seconds < 0) {
        seconds += SECONDS_PER_DAY;
        --days;
    }
    civilFromDays(days, year, month, day);
    const std::int64_t wholeMinutes = seconds / 60LL;
    hour = static_cast<std::uint8_t>((wholeMinutes / 60LL) % 24LL);
    minute = static_cast<std::uint8_t>(wholeMinutes % 60LL);
}


bool positionMovedSignificantly(
    double previousLatitude,
    double previousLongitude,
    double latitude,
    double longitude) {

    const double meanLatitude = (previousLatitude + latitude) * 0.5 * DEG_TO_RAD;
    const double northKm = (latitude - previousLatitude) * 111.32;
    const double eastKm = (longitude - previousLongitude) * 111.32 * std::cos(meanLatitude);
    return std::sqrt(northKm * northKm + eastKm * eastKm) >=
        POSITION_RECALCULATION_DISTANCE_KM;
}

double julianDay(std::int64_t utcEpoch) {
    return UNIX_EPOCH_JULIAN_DAY + static_cast<double>(utcEpoch) / 86400.0;
}

EquatorialPosition sunPosition(double jd) {
    const double d = jd - SCHLYTER_EPOCH_JULIAN_DAY;
    const double perihelion = normalizeDegrees(282.9404 + 4.70935e-5 * d);
    const double eccentricity = 0.016709 - 1.151e-9 * d;
    const double meanAnomaly = normalizeDegrees(356.0470 + 0.9856002585 * d);
    const double anomalyRadians = meanAnomaly * DEG_TO_RAD;
    const double eccentricAnomaly = meanAnomaly +
        eccentricity * RAD_TO_DEG * std::sin(anomalyRadians) *
        (1.0 + eccentricity * std::cos(anomalyRadians));
    const double eccentricRadians = eccentricAnomaly * DEG_TO_RAD;
    const double xv = std::cos(eccentricRadians) - eccentricity;
    const double yv = std::sqrt(1.0 - eccentricity * eccentricity) *
        std::sin(eccentricRadians);
    const double trueAnomaly = std::atan2(yv, xv) * RAD_TO_DEG;
    const double longitude = normalizeDegrees(trueAnomaly + perihelion);
    const double obliquity = (23.4393 - 3.563e-7 * d) * DEG_TO_RAD;
    const double longitudeRadians = longitude * DEG_TO_RAD;

    EquatorialPosition position;
    position.rightAscensionDegrees = normalizeDegrees(std::atan2(
        std::cos(obliquity) * std::sin(longitudeRadians),
        std::cos(longitudeRadians)) * RAD_TO_DEG);
    position.declinationDegrees = std::asin(
        std::sin(obliquity) * std::sin(longitudeRadians)) * RAD_TO_DEG;
    position.eclipticLongitudeDegrees = longitude;
    position.distanceEarthRadii = 0.0;
    return position;
}

EquatorialPosition moonPosition(double jd) {
    const double d = jd - SCHLYTER_EPOCH_JULIAN_DAY;
    const double ascendingNode = normalizeDegrees(125.1228 - 0.0529538083 * d);
    const double inclination = 5.1454;
    const double periapsis = normalizeDegrees(318.0634 + 0.1643573223 * d);
    const double semiMajorAxis = 60.2666;
    const double eccentricity = 0.054900;
    const double meanAnomaly = normalizeDegrees(115.3654 + 13.0649929509 * d);
    const double anomalyRadians = meanAnomaly * DEG_TO_RAD;
    const double eccentricAnomaly = meanAnomaly +
        eccentricity * RAD_TO_DEG * std::sin(anomalyRadians) *
        (1.0 + eccentricity * std::cos(anomalyRadians));
    const double eccentricRadians = eccentricAnomaly * DEG_TO_RAD;
    const double xv = semiMajorAxis * (std::cos(eccentricRadians) - eccentricity);
    const double yv = semiMajorAxis * std::sqrt(1.0 - eccentricity * eccentricity) *
        std::sin(eccentricRadians);
    const double trueAnomaly = std::atan2(yv, xv) * RAD_TO_DEG;
    double radius = std::sqrt(xv * xv + yv * yv);

    const double nodeRadians = ascendingNode * DEG_TO_RAD;
    const double orbitRadians = (trueAnomaly + periapsis) * DEG_TO_RAD;
    const double inclinationRadians = inclination * DEG_TO_RAD;
    const double xh = radius * (
        std::cos(nodeRadians) * std::cos(orbitRadians) -
        std::sin(nodeRadians) * std::sin(orbitRadians) * std::cos(inclinationRadians));
    const double yh = radius * (
        std::sin(nodeRadians) * std::cos(orbitRadians) +
        std::cos(nodeRadians) * std::sin(orbitRadians) * std::cos(inclinationRadians));
    const double zh = radius * std::sin(orbitRadians) * std::sin(inclinationRadians);
    double longitude = normalizeDegrees(std::atan2(yh, xh) * RAD_TO_DEG);
    double latitude = std::atan2(zh, std::sqrt(xh * xh + yh * yh)) * RAD_TO_DEG;

    const double sunPerihelion = normalizeDegrees(282.9404 + 4.70935e-5 * d);
    const double sunMeanAnomaly = normalizeDegrees(356.0470 + 0.9856002585 * d);
    const double sunMeanLongitude = normalizeDegrees(sunPerihelion + sunMeanAnomaly);
    const double moonMeanLongitude = normalizeDegrees(ascendingNode + periapsis + meanAnomaly);
    const double elongation = normalizeDegrees(moonMeanLongitude - sunMeanLongitude);
    const double argumentLatitude = normalizeDegrees(moonMeanLongitude - ascendingNode);

    longitude +=
        -1.274 * std::sin((meanAnomaly - 2.0 * elongation) * DEG_TO_RAD) +
         0.658 * std::sin((2.0 * elongation) * DEG_TO_RAD) -
         0.186 * std::sin(sunMeanAnomaly * DEG_TO_RAD) -
         0.059 * std::sin((2.0 * meanAnomaly - 2.0 * elongation) * DEG_TO_RAD) -
         0.057 * std::sin((meanAnomaly - 2.0 * elongation + sunMeanAnomaly) * DEG_TO_RAD) +
         0.053 * std::sin((meanAnomaly + 2.0 * elongation) * DEG_TO_RAD) +
         0.046 * std::sin((2.0 * elongation - sunMeanAnomaly) * DEG_TO_RAD) +
         0.041 * std::sin((meanAnomaly - sunMeanAnomaly) * DEG_TO_RAD) -
         0.035 * std::sin(elongation * DEG_TO_RAD) -
         0.031 * std::sin((meanAnomaly + sunMeanAnomaly) * DEG_TO_RAD) -
         0.015 * std::sin((2.0 * argumentLatitude - 2.0 * elongation) * DEG_TO_RAD) +
         0.011 * std::sin((meanAnomaly - 4.0 * elongation) * DEG_TO_RAD);
    latitude +=
        -0.173 * std::sin((argumentLatitude - 2.0 * elongation) * DEG_TO_RAD) -
         0.055 * std::sin((meanAnomaly - argumentLatitude - 2.0 * elongation) * DEG_TO_RAD) -
         0.046 * std::sin((meanAnomaly + argumentLatitude - 2.0 * elongation) * DEG_TO_RAD) +
         0.033 * std::sin((argumentLatitude + 2.0 * elongation) * DEG_TO_RAD) +
         0.017 * std::sin((2.0 * meanAnomaly + argumentLatitude) * DEG_TO_RAD);
    radius +=
        -0.58 * std::cos((meanAnomaly - 2.0 * elongation) * DEG_TO_RAD) -
         0.46 * std::cos((2.0 * elongation) * DEG_TO_RAD);

    longitude = normalizeDegrees(longitude);
    const double longitudeRadians = longitude * DEG_TO_RAD;
    const double latitudeRadians = latitude * DEG_TO_RAD;
    const double obliquity = (23.4393 - 3.563e-7 * d) * DEG_TO_RAD;
    const double xe = std::cos(longitudeRadians) * std::cos(latitudeRadians);
    const double ye = std::sin(longitudeRadians) * std::cos(latitudeRadians) * std::cos(obliquity) -
        std::sin(latitudeRadians) * std::sin(obliquity);
    const double ze = std::sin(longitudeRadians) * std::cos(latitudeRadians) * std::sin(obliquity) +
        std::sin(latitudeRadians) * std::cos(obliquity);

    EquatorialPosition position;
    position.rightAscensionDegrees = normalizeDegrees(std::atan2(ye, xe) * RAD_TO_DEG);
    position.declinationDegrees = std::atan2(ze, std::sqrt(xe * xe + ye * ye)) * RAD_TO_DEG;
    position.eclipticLongitudeDegrees = longitude;
    position.distanceEarthRadii = radius;
    return position;
}

double altitudeDegrees(
    const EquatorialPosition& position,
    double jd,
    double latitudeDegrees,
    double longitudeDegrees) {

    const double centuries = (jd - J2000_JULIAN_DAY) / 36525.0;
    const double greenwichSidereal = normalizeDegrees(
        280.46061837 + 360.98564736629 * (jd - J2000_JULIAN_DAY) +
        0.000387933 * centuries * centuries -
        centuries * centuries * centuries / 38710000.0);
    const double hourAngle = normalizeSignedDegrees(
        greenwichSidereal + longitudeDegrees - position.rightAscensionDegrees) * DEG_TO_RAD;
    const double latitude = latitudeDegrees * DEG_TO_RAD;
    const double declination = position.declinationDegrees * DEG_TO_RAD;
    const double sineAltitude =
        std::sin(latitude) * std::sin(declination) +
        std::cos(latitude) * std::cos(declination) * std::cos(hourAngle);
    return std::asin(std::max(-1.0, std::min(1.0, sineAltitude))) * RAD_TO_DEG;
}

double sunCrossingValue(std::int64_t epoch, double latitude, double longitude) {
    const double jd = julianDay(epoch);
    return altitudeDegrees(sunPosition(jd), jd, latitude, longitude) + 0.833;
}

double moonCrossingValue(std::int64_t epoch, double latitude, double longitude) {
    const double jd = julianDay(epoch);
    const EquatorialPosition moon = moonPosition(jd);
    const double clampedDistance = std::max(1.01, moon.distanceEarthRadii);
    const double horizontalParallax = std::asin(1.0 / clampedDistance) * RAD_TO_DEG;
    const double standardAltitude = 0.7275 * horizontalParallax - 0.5667;
    return altitudeDegrees(moon, jd, latitude, longitude) - standardAltitude;
}

using CrossingFunction = double (*)(std::int64_t, double, double);

std::int64_t refineCrossing(
    std::int64_t left,
    std::int64_t right,
    double latitude,
    double longitude,
    CrossingFunction function) {

    double leftValue = function(left, latitude, longitude);
    for (std::uint8_t iteration = 0; iteration < 24U; ++iteration) {
        const std::int64_t middle = left + (right - left) / 2LL;
        const double middleValue = function(middle, latitude, longitude);
        if ((leftValue < 0.0 && middleValue < 0.0) ||
            (leftValue >= 0.0 && middleValue >= 0.0)) {
            left = middle;
            leftValue = middleValue;
        } else {
            right = middle;
        }
    }
    return left + (right - left) / 2LL;
}

bool eventBelongsToDate(
    std::int64_t utcEpoch,
    std::uint16_t targetYear,
    std::uint8_t targetMonth,
    std::uint8_t targetDay) {

    std::uint16_t year = 0;
    std::uint8_t month = 0;
    std::uint8_t day = 0;
    std::uint8_t hour = 0;
    std::uint8_t minute = 0;
    localDateFromUtc(utcEpoch, year, month, day, hour, minute);
    return year == targetYear && month == targetMonth && day == targetDay;
}

CrossingSummary findCrossings(
    std::uint16_t year,
    std::uint8_t month,
    std::uint8_t day,
    double latitude,
    double longitude,
    CrossingFunction function) {

    CrossingSummary summary;
    const std::int64_t nominalLocalMidnight = civilEpoch(year, month, day);
    const std::int64_t searchStart = nominalLocalMidnight - SEARCH_MARGIN_SECONDS;
    const std::int64_t searchEnd = nominalLocalMidnight + SECONDS_PER_DAY + SEARCH_MARGIN_SECONDS;
    std::int64_t previousEpoch = searchStart;
    double previousValue = function(previousEpoch, latitude, longitude);
    bool allAbove = previousValue >= 0.0;
    bool allBelow = previousValue < 0.0;

    for (std::int64_t epoch = searchStart + SEARCH_STEP_SECONDS;
         epoch <= searchEnd;
         epoch += SEARCH_STEP_SECONDS) {
        const double value = function(epoch, latitude, longitude);
        allAbove = allAbove && value >= 0.0;
        allBelow = allBelow && value < 0.0;
        const bool rising = previousValue < 0.0 && value >= 0.0;
        const bool setting = previousValue >= 0.0 && value < 0.0;
        if (rising || setting) {
            const std::int64_t crossing = refineCrossing(
                previousEpoch,
                epoch,
                latitude,
                longitude,
                function);
            if (eventBelongsToDate(crossing, year, month, day)) {
                if (rising && !summary.riseValid) {
                    summary.riseValid = true;
                    summary.riseUtcEpoch = crossing;
                } else if (setting && !summary.setValid) {
                    summary.setValid = true;
                    summary.setUtcEpoch = crossing;
                }
            }
        }
        previousEpoch = epoch;
        previousValue = value;
    }

    summary.aboveAllDay = !summary.riseValid && !summary.setValid && allAbove;
    summary.belowAllDay = !summary.riseValid && !summary.setValid && allBelow;
    return summary;
}

AstronomyService::EventTime localEventTime(std::int64_t utcEpoch) {
    AstronomyService::EventTime event;
    std::uint16_t year = 0;
    std::uint8_t month = 0;
    std::uint8_t day = 0;
    localDateFromUtc(utcEpoch, year, month, day, event.hour, event.minute);
    event.valid = true;
    return event;
}

AstronomyService::MoonPhase phaseFromElongation(double elongationDegrees) {
    const double phase = normalizeDegrees(elongationDegrees);
    if (phase < 22.5 || phase >= 337.5) return AstronomyService::MoonPhase::NewMoon;
    if (phase < 67.5) return AstronomyService::MoonPhase::WaxingCrescent;
    if (phase < 112.5) return AstronomyService::MoonPhase::FirstQuarter;
    if (phase < 157.5) return AstronomyService::MoonPhase::WaxingGibbous;
    if (phase < 202.5) return AstronomyService::MoonPhase::FullMoon;
    if (phase < 247.5) return AstronomyService::MoonPhase::WaningGibbous;
    if (phase < 292.5) return AstronomyService::MoonPhase::LastQuarter;
    return AstronomyService::MoonPhase::WaningCrescent;
}

}  // namespace

void AstronomyService::update(
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
    const PositionReference& reference) {

    const bool utcValid = validDate(utcYear, utcMonth, utcDay) &&
        utcHour < 24U && utcMinute < 60U && utcSecond < 60U;
    const bool inputValid = timeValid && utcValid && reference.valid &&
        validDate(localYear, localMonth, localDay) &&
        std::isfinite(reference.latitude) && std::isfinite(reference.longitude) &&
        reference.latitude >= -90.0 && reference.latitude <= 90.0 &&
        reference.longitude >= -180.0 && reference.longitude <= 180.0;

    if (!inputValid) {
        const bool changed = view_.valid || view_.timeValid != timeValid ||
            view_.positionValid != reference.valid;
        view_.valid = false;
        view_.timeValid = timeValid;
        view_.positionValid = reference.valid;
        view_.positionFromGps = reference.fromGps;
        observedPositionValid_ = false;
        observedYear_ = 0;
        observedMonth_ = 0;
        observedDay_ = 0;
        dynamicUpdateInitialized_ = false;
        if (changed) {
            ++view_.revision;
        }
        return;
    }

    const bool dateChanged = observedYear_ != localYear ||
        observedMonth_ != localMonth || observedDay_ != localDay;
    const bool sourceChanged = !observedPositionValid_ ||
        observedPositionFromGps_ != reference.fromGps;
    const bool positionChanged = !observedPositionValid_ || positionMovedSignificantly(
        observedLatitude_,
        observedLongitude_,
        reference.latitude,
        reference.longitude);
    const bool geometryChanged = dateChanged || sourceChanged || positionChanged;
    const bool dynamicDue = !dynamicUpdateInitialized_ ||
        static_cast<std::uint32_t>(now - lastDynamicUpdateAt_) >= DYNAMIC_UPDATE_INTERVAL_MS;
    if (!geometryChanged && !dynamicDue) {
        return;
    }

    ViewState next = geometryChanged ? ViewState{} : view_;
    next.valid = true;
    next.timeValid = true;
    next.positionValid = true;
    next.positionFromGps = reference.fromGps;
    next.year = localYear;
    next.month = localMonth;
    next.day = localDay;
    next.latitude = reference.latitude;
    next.longitude = reference.longitude;

    if (geometryChanged) {
        observedPositionValid_ = true;
        observedPositionFromGps_ = reference.fromGps;
        observedLatitude_ = reference.latitude;
        observedLongitude_ = reference.longitude;
        observedYear_ = localYear;
        observedMonth_ = localMonth;
        observedDay_ = localDay;

        const CrossingSummary sun = findCrossings(
            localYear,
            localMonth,
            localDay,
            reference.latitude,
            reference.longitude,
            sunCrossingValue);
        const CrossingSummary moon = findCrossings(
            localYear,
            localMonth,
            localDay,
            reference.latitude,
            reference.longitude,
            moonCrossingValue);

        next.sunAboveAllDay = sun.aboveAllDay;
        next.sunBelowAllDay = sun.belowAllDay;
        next.moonAboveAllDay = moon.aboveAllDay;
        next.moonBelowAllDay = moon.belowAllDay;
        if (sun.riseValid) next.sunrise = localEventTime(sun.riseUtcEpoch);
        if (sun.setValid) next.sunset = localEventTime(sun.setUtcEpoch);
        if (moon.riseValid) next.moonrise = localEventTime(moon.riseUtcEpoch);
        if (moon.setValid) next.moonset = localEventTime(moon.setUtcEpoch);
        if (sun.riseValid && sun.setValid && sun.setUtcEpoch > sun.riseUtcEpoch) {
            next.daylightMinutes = static_cast<std::uint16_t>(
                (sun.setUtcEpoch - sun.riseUtcEpoch + 30LL) / 60LL);
        }
    }

    const std::int64_t utcEpoch = civilEpoch(
        utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);
    const double currentJd = julianDay(utcEpoch);
    const EquatorialPosition currentSun = sunPosition(currentJd);
    const double elongation = normalizeDegrees(
        moonPosition(currentJd).eclipticLongitudeDegrees -
        currentSun.eclipticLongitudeDegrees);
    const double illumination = 0.5 * (1.0 - std::cos(elongation * DEG_TO_RAD));

    next.sunAltitudeDegrees = static_cast<float>(altitudeDegrees(
        currentSun, currentJd, reference.latitude, reference.longitude));
    next.moonElongationDegrees = static_cast<float>(elongation);
    next.moonPhase = phaseFromElongation(elongation);
    next.moonIlluminationPercent = static_cast<std::uint8_t>(std::max(
        0.0,
        std::min(100.0, std::round(illumination * 100.0))));
    next.moonAgeDays = static_cast<float>(elongation / 360.0 * SYNODIC_MONTH_DAYS);
    next.revision = view_.revision + 1U;
    view_ = next;
    lastDynamicUpdateAt_ = now;
    dynamicUpdateInitialized_ = true;
}

const AstronomyService::ViewState& AstronomyService::viewState() const {
    return view_;
}

const char* moonPhaseTextCzech(AstronomyService::MoonPhase phase) {
    switch (phase) {
        case AstronomyService::MoonPhase::NewMoon: return "Nov";
        case AstronomyService::MoonPhase::WaxingCrescent: return "Dorustajici srpek";
        case AstronomyService::MoonPhase::FirstQuarter: return "Prvni ctvrt";
        case AstronomyService::MoonPhase::WaxingGibbous: return "Dorustajici Mesic";
        case AstronomyService::MoonPhase::FullMoon: return "Uplnek";
        case AstronomyService::MoonPhase::WaningGibbous: return "Couvajici Mesic";
        case AstronomyService::MoonPhase::LastQuarter: return "Posledni ctvrt";
        case AstronomyService::MoonPhase::WaningCrescent: return "Couvajici srpek";
        default: return "--";
    }
}

const char* moonPhaseTextEnglish(AstronomyService::MoonPhase phase) {
    switch (phase) {
        case AstronomyService::MoonPhase::NewMoon: return "New moon";
        case AstronomyService::MoonPhase::WaxingCrescent: return "Waxing crescent";
        case AstronomyService::MoonPhase::FirstQuarter: return "First quarter";
        case AstronomyService::MoonPhase::WaxingGibbous: return "Waxing gibbous";
        case AstronomyService::MoonPhase::FullMoon: return "Full moon";
        case AstronomyService::MoonPhase::WaningGibbous: return "Waning gibbous";
        case AstronomyService::MoonPhase::LastQuarter: return "Last quarter";
        case AstronomyService::MoonPhase::WaningCrescent: return "Waning crescent";
        default: return "--";
    }
}

}  // namespace Services
