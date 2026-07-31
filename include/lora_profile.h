#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace LoRaProfile {

// Czech LoRa APRS profile used by the existing network.
constexpr float FREQUENCY_MHZ = 433.775F;
constexpr float BANDWIDTH_KHZ = 125.0F;
constexpr std::uint8_t SPREADING_FACTOR = 12;
constexpr std::uint8_t CODING_RATE = 5;  // 4/5
constexpr std::uint8_t SYNC_WORD = 0x12;
constexpr std::int8_t OUTPUT_POWER_DBM = 10;
constexpr std::uint16_t PREAMBLE_LENGTH = 8;
constexpr std::uint8_t GAIN = 0;  // Automatic gain control.
constexpr std::uint8_t CURRENT_LIMIT_MA = 100;
constexpr std::uint32_t SPI_FREQUENCY_HZ = 4000000;

// RA-02 / SX1278 limits exposed by the Settings screen. The sync word,
// preamble, explicit header and disabled RadioLib CRC stay fixed so custom
// profiles remain compatible with the terminal packet format.
constexpr float MIN_FREQUENCY_MHZ = 410.0F;
constexpr float MAX_FREQUENCY_MHZ = 525.0F;
constexpr std::uint8_t MIN_SPREADING_FACTOR = 7;
constexpr std::uint8_t MAX_SPREADING_FACTOR = 12;
constexpr std::uint8_t MIN_CODING_RATE = 5;
constexpr std::uint8_t MAX_CODING_RATE = 8;
constexpr std::int8_t MIN_OUTPUT_POWER_DBM = 2;
constexpr std::int8_t MAX_OUTPUT_POWER_DBM = 17;

constexpr std::size_t MAX_PACKET_LENGTH = 255;

struct Config {
    float frequencyMHz = FREQUENCY_MHZ;
    float bandwidthKHz = BANDWIDTH_KHZ;
    std::uint8_t spreadingFactor = SPREADING_FACTOR;
    std::uint8_t codingRate = CODING_RATE;
    std::int8_t outputPowerDbm = OUTPUT_POWER_DBM;
};

inline Config czeAprsConfig() {
    return Config{};
}

inline bool isSupportedBandwidth(float value) {
    return std::fabs(value - 62.5F) < 0.01F ||
        std::fabs(value - 125.0F) < 0.01F ||
        std::fabs(value - 250.0F) < 0.01F ||
        std::fabs(value - 500.0F) < 0.01F;
}

inline bool isValidConfig(const Config& config) {
    return std::isfinite(config.frequencyMHz) &&
        config.frequencyMHz >= MIN_FREQUENCY_MHZ &&
        config.frequencyMHz <= MAX_FREQUENCY_MHZ &&
        isSupportedBandwidth(config.bandwidthKHz) &&
        config.spreadingFactor >= MIN_SPREADING_FACTOR &&
        config.spreadingFactor <= MAX_SPREADING_FACTOR &&
        config.codingRate >= MIN_CODING_RATE &&
        config.codingRate <= MAX_CODING_RATE &&
        config.outputPowerDbm >= MIN_OUTPUT_POWER_DBM &&
        config.outputPowerDbm <= MAX_OUTPUT_POWER_DBM;
}

inline bool sameConfig(const Config& left, const Config& right) {
    return std::fabs(left.frequencyMHz - right.frequencyMHz) < 0.0005F &&
        std::fabs(left.bandwidthKHz - right.bandwidthKHz) < 0.01F &&
        left.spreadingFactor == right.spreadingFactor &&
        left.codingRate == right.codingRate &&
        left.outputPowerDbm == right.outputPowerDbm;
}

}  // namespace LoRaProfile
