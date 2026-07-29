#pragma once

#include <cstdint>

namespace AppConfig {

constexpr char FIRMWARE_NAME[] = "LoRa APRS Terminal";
constexpr char FIRMWARE_VERSION[] = "1.0.5";
constexpr char BOARD_NAME[] = "Waveshare ESP32-Touch-LCD-3.5";

// Defaults used when no values have yet been saved in NVS.
constexpr char DEFAULT_CALLSIGN[] = "OK5TVR-15";
constexpr double DEFAULT_LATITUDE = 49.786333;
constexpr double DEFAULT_LONGITUDE = 13.285000;
constexpr char APRS_DESTINATION[] = "APRS";
constexpr char TRACKER_COMMENT[] = "LoRa tracker";
constexpr char DEFAULT_APRS_IS_SERVER[] = "rotate.aprs2.net";
constexpr std::uint16_t DEFAULT_APRS_IS_PORT = 14580;
constexpr char DEFAULT_APRS_IS_FILTER[] = "";

constexpr std::uint32_t SERIAL_BAUD_RATE = 115200;
constexpr std::uint32_t GPS_BAUD_RATE = 9600;
constexpr std::uint32_t GPS_DETECT_TIMEOUT_MS = 10000;
constexpr std::uint32_t GPS_FIX_MAX_AGE_MS = 5000;
constexpr std::uint32_t SPLASH_DURATION_MS = 1800;
constexpr std::uint16_t LVGL_BUFFER_LINES = 12;
constexpr std::uint32_t MAIN_LOOP_DELAY_MS = 2;
constexpr std::uint32_t UI_REFRESH_INTERVAL_MS = 250;
constexpr std::uint32_t RADIO_TX_TIMEOUT_MS = 15000;
constexpr std::uint32_t SD_SPI_FREQUENCY_HZ = 20000000;

constexpr std::uint32_t TRACKER_DEFAULT_INTERVAL_SECONDS = 300;
constexpr std::uint32_t TRACKER_MIN_INTERVAL_SECONDS = 30;
constexpr std::uint32_t TRACKER_MAX_INTERVAL_SECONDS = 3600;
constexpr std::uint32_t TRACKER_START_DELAY_MS = 2000;
constexpr std::uint32_t MANUAL_BEACON_REQUEST_TIMEOUT_MS = 15000;

constexpr std::uint32_t BUTTON_DEBOUNCE_MS = 35;
constexpr std::uint32_t BUTTON_MIN_CLICK_MS = 40;
constexpr std::uint32_t BUTTON_MAX_CLICK_MS = 1500;
constexpr std::uint32_t BUTTON_CLICK_COOLDOWN_MS = 1000;

constexpr std::uint32_t DIGI_DUPLICATE_WINDOW_MS = 30000;
constexpr std::uint32_t DIGI_MIN_DELAY_MS = 120;
constexpr std::uint32_t DIGI_MAX_DELAY_MS = 420;
constexpr std::uint32_t WIFI_RECONNECT_INTERVAL_MS = 15000;
constexpr std::uint32_t APRS_IS_RECONNECT_INTERVAL_MS = 10000;
constexpr std::uint32_t APRS_IS_KEEPALIVE_TIMEOUT_MS = 90000;
constexpr std::uint32_t APRS_IS_CONNECT_TIMEOUT_MS = 1500;
constexpr std::uint32_t APRS_IS_GATE_MAX_AGE_MS = 30000;

// SmartBeacon defaults use km/h and seconds.
constexpr float SMARTBEACON_LOW_SPEED_KMH = 5.0F;
constexpr float SMARTBEACON_HIGH_SPEED_KMH = 70.0F;
constexpr std::uint32_t SMARTBEACON_SLOW_RATE_SECONDS = 1800;
constexpr std::uint32_t SMARTBEACON_FAST_RATE_SECONDS = 120;
constexpr std::uint32_t SMARTBEACON_MIN_TURN_SECONDS = 15;
constexpr float SMARTBEACON_TURN_ANGLE_DEGREES = 30.0F;
constexpr float SMARTBEACON_TURN_SLOPE = 240.0F;

constexpr bool ENABLE_LORA = true;
constexpr bool ENABLE_SD_CARD = true;
constexpr bool ENABLE_GPS = true;
constexpr bool ENABLE_OE_LORA_APRS_HEADER = true;

}  // namespace AppConfig
