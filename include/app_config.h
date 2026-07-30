#pragma once

#include <cstddef>
#include <cstdint>

namespace AppConfig {

constexpr char FIRMWARE_NAME[] = "LoRa APRS Terminal";
constexpr char FIRMWARE_VERSION[] = "2.0.0";
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
constexpr std::uint32_t RADIO_TX_MIN_GAP_MS = 180;
constexpr std::uint32_t RADIO_RECOVERY_RETRY_MS = 5000;
constexpr std::uint8_t RADIO_RECOVERY_RX_ERROR_THRESHOLD = 3;

// Background RSSI monitoring on the configured LoRa channel. One stored point
// is the average of a short, non-blocking burst of instantaneous RSSI reads.
constexpr std::uint32_t RADIO_NOISE_INITIAL_DELAY_MS = 15000;
constexpr std::uint32_t RADIO_NOISE_SAMPLE_INTERVAL_MS = 300000;
constexpr std::uint32_t RADIO_NOISE_RETRY_DELAY_MS = 5000;
constexpr std::uint32_t RADIO_NOISE_BURST_SPACING_MS = 25;
constexpr std::uint8_t RADIO_NOISE_BURST_SAMPLES = 8;
constexpr std::size_t RADIO_NOISE_HISTORY_LENGTH = 20;
static_assert(RADIO_NOISE_HISTORY_LENGTH > 0 && RADIO_NOISE_HISTORY_LENGTH <= 255,
              "RSSI history length must fit the ViewState counter");
constexpr std::uint32_t SD_SPI_FREQUENCY_HZ = 20000000;

// Offline map. Tiles use standard Web Mercator XYZ coordinates and are stored
// as 256 x 256 little-endian RGB565 files: /MAP/<z>/<x>/<y>.rgb.
constexpr char MAP_DIRECTORY[] = "/MAP";
constexpr std::uint8_t MAP_DEFAULT_ZOOM = 13;
constexpr std::uint8_t MAP_MIN_ZOOM = 3;
constexpr std::uint8_t MAP_MAX_ZOOM = 18;
constexpr std::uint16_t MAP_RECENTER_THRESHOLD_PIXELS = 64;
constexpr std::uint16_t MAP_TILE_ROWS_PER_UPDATE = 8;
constexpr std::size_t MAP_RECENT_TRAIL_POINTS = 64;
static_assert(MAP_RECENT_TRAIL_POINTS > 1 && MAP_RECENT_TRAIL_POINTS <= 255,
              "Map trail history must fit the uint8_t counter");

// AXP2101 power telemetry. Values are read-only; the firmware does not alter
// charger current, target voltage, or PMIC output rails.
constexpr std::uint32_t POWER_POLL_INTERVAL_MS = 2000;
constexpr std::uint8_t POWER_CRITICAL_PERCENT = 10;
constexpr std::uint16_t POWER_CRITICAL_VOLTAGE_MV = 3400;

constexpr std::uint32_t TRACKER_DEFAULT_INTERVAL_SECONDS = 300;
constexpr std::uint32_t TRACKER_MIN_INTERVAL_SECONDS = 30;
constexpr std::uint32_t TRACKER_MAX_INTERVAL_SECONDS = 3600;
constexpr std::uint32_t TRACKER_START_DELAY_MS = 2000;
constexpr std::uint32_t MANUAL_BEACON_REQUEST_TIMEOUT_MS = 15000;

// Stopař route logger. SD writes are intentionally sparse and buffered so
// radio RX and APRS tracker processing remain the highest-priority loop work.
constexpr std::uint32_t TRAIL_SAMPLE_INTERVAL_MS = 5000;
constexpr std::uint32_t TRAIL_MAX_POINT_INTERVAL_MS = 30000;
constexpr double TRAIL_MIN_POINT_DISTANCE_METERS = 3.0;
constexpr std::uint32_t TRAIL_AUTOPAUSE_DELAY_MS = 30000;
constexpr float TRAIL_RESUME_SPEED_KMH = 2.0F;
constexpr double TRAIL_RESUME_DISTANCE_METERS = 8.0;
constexpr double TRAIL_MAX_POINT_JUMP_KM = 2.0;
constexpr std::uint32_t TRAIL_FLUSH_INTERVAL_MS = 15000;
constexpr std::uint32_t TRAIL_FLUSH_AFTER_LINES = 8;

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
