#include "services/weather_store.h"

#include <cstring>

namespace Services {

void WeatherStore::clear() {
    view_ = ViewState{};
}

bool WeatherStore::ingest(
    const Aprs::ParsedFrame& frame,
    float rssiDbm,
    float snrDb,
    std::uint32_t now) {

    if (!frame.valid || !frame.weather.valid || frame.source[0] == '\0') {
        return false;
    }

    std::size_t existingIndex = view_.count;
    for (std::size_t index = 0; index < view_.count; ++index) {
        if (sameCallsign(view_.stations[index].callsign, frame.source)) {
            existingIndex = index;
            break;
        }
    }

    WeatherStation updated;
    if (existingIndex < view_.count) {
        updated = view_.stations[existingIndex];
        for (std::size_t index = existingIndex; index + 1 < view_.count; ++index) {
            view_.stations[index] = view_.stations[index + 1];
        }
    } else if (view_.count < MAX_STATIONS) {
        ++view_.count;
    }

    // Newest record is always index 0. Once the list is full, the record at
    // the last index is naturally overwritten by this shift.
    for (std::size_t index = view_.count - 1; index > 0; --index) {
        view_.stations[index] = view_.stations[index - 1];
    }

    updated.used = true;
    std::strncpy(updated.callsign, frame.source, sizeof(updated.callsign) - 1);
    updated.callsign[sizeof(updated.callsign) - 1] = '\0';
    updated.lastRssiDbm = rssiDbm;
    updated.lastSnrDb = snrDb;
    updated.lastHeardMs = now;
    ++updated.heardCount;

    if (frame.hasPosition) {
        updated.hasPosition = true;
        updated.latitude = frame.latitude;
        updated.longitude = frame.longitude;
        updated.symbol[0] = frame.symbolTable;
        updated.symbol[1] = frame.symbolCode;
        updated.symbol[2] = '\0';
    }

    const Aprs::WeatherData& weather = frame.weather;
    if (weather.hasTemperature) {
        updated.hasTemperature = true;
        updated.temperatureC = weather.temperatureC;
    }
    if (weather.hasHumidity) {
        updated.hasHumidity = true;
        updated.humidityPercent = weather.humidityPercent;
    }
    if (weather.hasPressure) {
        updated.hasPressure = true;
        updated.pressureHpa = weather.pressureHpa;
    }
    if (weather.hasWindDirection) {
        updated.hasWindDirection = true;
        updated.windDirectionDegrees = weather.windDirectionDegrees;
    }
    if (weather.hasWindSpeed) {
        updated.hasWindSpeed = true;
        updated.windSpeedKmh = weather.windSpeedKmh;
    }
    if (weather.hasWindGust) {
        updated.hasWindGust = true;
        updated.windGustKmh = weather.windGustKmh;
    }
    if (weather.hasRainLastHour) {
        updated.hasRainLastHour = true;
        updated.rainLastHourMm = weather.rainLastHourMm;
    }
    if (weather.hasRainLast24Hours) {
        updated.hasRainLast24Hours = true;
        updated.rainLast24HoursMm = weather.rainLast24HoursMm;
    }
    if (weather.hasRainToday) {
        updated.hasRainToday = true;
        updated.rainTodayMm = weather.rainTodayMm;
    }
    if (weather.hasSolarRadiation) {
        updated.hasSolarRadiation = true;
        updated.solarRadiationWm2 = weather.solarRadiationWm2;
    }

    view_.stations[0] = updated;
    ++view_.revision;
    return true;
}

const WeatherStore::ViewState& WeatherStore::viewState() const {
    return view_;
}

bool WeatherStore::sameCallsign(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return false;
    }
    while (*left != '\0' && *right != '\0') {
        char a = *left;
        char b = *right;
        if (a >= 'a' && a <= 'z') {
            a = static_cast<char>(a - 'a' + 'A');
        }
        if (b >= 'a' && b <= 'z') {
            b = static_cast<char>(b - 'a' + 'A');
        }
        if (a != b) {
            return false;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

}  // namespace Services
