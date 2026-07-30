#pragma once

#include <aprs_codec.h>
#include <cstddef>
#include <cstdint>

namespace Services {

class WeatherStore {
public:
    static constexpr std::size_t MAX_STATIONS = 5;

    struct WeatherStation {
        bool used = false;
        char callsign[Aprs::MAX_SOURCE_CALL_LENGTH + 1] = {};
        char symbol[3] = {'-', '-', '\0'};
        bool hasPosition = false;
        double latitude = 0.0;
        double longitude = 0.0;

        bool hasTemperature = false;
        bool hasHumidity = false;
        bool hasPressure = false;
        bool hasWindDirection = false;
        bool hasWindSpeed = false;
        bool hasWindGust = false;
        bool hasRainLastHour = false;
        bool hasRainLast24Hours = false;
        bool hasRainToday = false;
        bool hasSolarRadiation = false;

        float temperatureC = 0.0F;
        float humidityPercent = 0.0F;
        float pressureHpa = 0.0F;
        float windDirectionDegrees = 0.0F;
        float windSpeedKmh = 0.0F;
        float windGustKmh = 0.0F;
        float rainLastHourMm = 0.0F;
        float rainLast24HoursMm = 0.0F;
        float rainTodayMm = 0.0F;
        float solarRadiationWm2 = 0.0F;

        float lastRssiDbm = 0.0F;
        float lastSnrDb = 0.0F;
        std::uint32_t lastHeardMs = 0;
        std::uint32_t heardCount = 0;
    };

    struct ViewState {
        WeatherStation stations[MAX_STATIONS] = {};
        std::size_t count = 0;
        std::uint32_t revision = 0;
    };

    void clear();
    bool ingest(
        const Aprs::ParsedFrame& frame,
        float rssiDbm,
        float snrDb,
        std::uint32_t now);
    const ViewState& viewState() const;

private:
    static bool sameCallsign(const char* left, const char* right);

    ViewState view_;
};

}  // namespace Services
