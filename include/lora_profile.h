#pragma once

#include <cstddef>
#include <cstdint>

namespace LoRaProfile {

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

constexpr std::size_t MAX_PACKET_LENGTH = 255;

}  // namespace LoRaProfile
