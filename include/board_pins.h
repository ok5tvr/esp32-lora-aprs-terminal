#pragma once

#include <Arduino.h>
#include <cstdint>

namespace BoardPins {

// Waveshare ESP32-Touch-LCD-3.5 with classic ESP32-D0WDR2-V3.
// LCD and microSD share the VSPI data lines and use separate chip-select pins.
constexpr int LCD_MOSI = 23;
constexpr int LCD_MISO = 19;
constexpr int LCD_DC = 27;
constexpr int LCD_SCLK = 18;
constexpr int LCD_BACKLIGHT = 25;
constexpr int LCD_CS = 5;
constexpr int LCD_RESET = -1;  // Reset is controlled through TCA9554.

constexpr int SD_MOSI = 23;
constexpr int SD_MISO = 19;
constexpr int SD_SCLK = 18;
constexpr int SD_CS = 15;

constexpr int I2C_SDA = 21;
constexpr int I2C_SCL = 22;
constexpr std::uint8_t TCA9554_ADDRESS = 0x20;
constexpr std::uint8_t TOUCH_ADDRESS = 0x38;
constexpr int PMU_IRQ = 35;  // AXP2101 interrupt output (input-only GPIO).
constexpr std::uint8_t TCA_RESET_OUTPUT_0 = 0;
constexpr std::uint8_t TCA_RESET_OUTPUT_1 = 1;

// Ai-Thinker RA-02 / SX1278 on a dedicated HSPI bus.
// This keeps LoRa independent from the LCD + microSD VSPI bus.
constexpr int LORA_MISO = 13;
constexpr int LORA_SCLK = 14;
constexpr int LORA_MOSI = 26;
constexpr int LORA_CS = 33;
constexpr int LORA_RESET = 32;
// GPIO2 is a boot-strapping pin. RA-02 DIO0 is normally low during reset;
// add an external 10 kOhm pulldown to GND for reliable firmware upload.
// The optional onboard I2S audio must remain disabled because it also uses GPIO2.
constexpr int LORA_DIO0 = 2;
constexpr int LORA_DIO1 = -1;  // DIO0 is sufficient for RX/TX interrupts.

// Optional NMEA GPS receiver. Only the GPS TX output is required and is
// connected to ESP32 GPIO4 (UART2 RX). GPIO4 is also an optional I2S audio
// signal on this board, therefore the onboard audio interface must remain disabled.
// The ESP32 TX line is intentionally unused.
constexpr int GPS_RX = 4;
constexpr int GPS_TX = -1;

static_assert(GPS_RX != LORA_DIO0, "GPS RX and LoRa DIO0 must use different GPIOs");
static_assert(GPS_RX != LORA_CS, "GPS RX conflicts with LoRa chip select");
static_assert(GPS_RX != LORA_RESET, "GPS RX conflicts with LoRa reset");

// Onboard BOOT button. It is a boot-strapping input only during reset; while
// the firmware is running it can be read as a normal active-low button.
// RESET is connected to CHIP_PU and is not software-readable. The PWR button
// keeps its power-management role and is deliberately not used by this app.
constexpr int BOOT_BUTTON = 0;

constexpr std::uint16_t LCD_NATIVE_WIDTH = 320;
constexpr std::uint16_t LCD_NATIVE_HEIGHT = 480;
constexpr std::uint16_t SCREEN_WIDTH = 480;
constexpr std::uint16_t SCREEN_HEIGHT = 320;
constexpr std::uint8_t DISPLAY_ROTATION = 1;

// FT6336 raw coordinates match portrait rotation 0. For rotation 1:
// screen X = raw Y, screen Y = 319 - raw X.
constexpr bool TOUCH_SWAP_XY = true;
constexpr bool TOUCH_MIRROR_X = false;
constexpr bool TOUCH_MIRROR_Y = true;

}  // namespace BoardPins
