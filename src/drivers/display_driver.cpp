#include "drivers/display_driver.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <TCA9554.h>
#include <Wire.h>
#include <esp_arduino_version.h>

#include "app_config.h"
#include "app_log.h"
#include "board_pins.h"

namespace Drivers {
namespace Display {
namespace {

// Explicit VSPI selection is important: RA-02 uses the other hardware bus (HSPI).
// The last argument marks the LCD bus as shared because the onboard microSD card
// uses the same SCK/MISO/MOSI lines with a different CS pin.
Arduino_ESP32SPI displayBus(
    BoardPins::LCD_DC,
    BoardPins::LCD_CS,
    BoardPins::LCD_SCLK,
    BoardPins::LCD_MOSI,
    BoardPins::LCD_MISO,
    VSPI,
    true);

Arduino_ST7796 display(
    &displayBus,
    BoardPins::LCD_RESET,
    0,
    true,
    BoardPins::LCD_NATIVE_WIDTH,
    BoardPins::LCD_NATIVE_HEIGHT);

TCA9554 ioExpander(BoardPins::TCA9554_ADDRESS);
std::uint8_t currentBacklightPercent = 100;
bool backlightPwmReady = false;

std::uint32_t dutyFromPercent(std::uint8_t percent) {
    const std::uint32_t maxDuty =
        (1UL << AppConfig::DISPLAY_BACKLIGHT_PWM_RESOLUTION_BITS) - 1UL;
    return (maxDuty * static_cast<std::uint32_t>(percent) + 50UL) / 100UL;
}

void resetDisplayAndTouch() {
    ioExpander.pinMode1(BoardPins::TCA_RESET_OUTPUT_0, OUTPUT);
    ioExpander.pinMode1(BoardPins::TCA_RESET_OUTPUT_1, OUTPUT);

    ioExpander.write1(BoardPins::TCA_RESET_OUTPUT_0, HIGH);
    ioExpander.write1(BoardPins::TCA_RESET_OUTPUT_1, HIGH);
    delay(10);
    ioExpander.write1(BoardPins::TCA_RESET_OUTPUT_0, LOW);
    ioExpander.write1(BoardPins::TCA_RESET_OUTPUT_1, LOW);
    delay(10);
    ioExpander.write1(BoardPins::TCA_RESET_OUTPUT_0, HIGH);
    ioExpander.write1(BoardPins::TCA_RESET_OUTPUT_1, HIGH);
    delay(200);
}

}  // namespace

bool begin() {
    // Keep the SD card deselected while the display is initialized.
    pinMode(BoardPins::SD_CS, OUTPUT);
    digitalWrite(BoardPins::SD_CS, HIGH);

    Wire.begin(BoardPins::I2C_SDA, BoardPins::I2C_SCL);
    Wire.setClock(400000);

    // The official Waveshare example initializes the expander without
    // depending on a library-specific return value.
    ioExpander.begin();
    resetDisplayAndTouch();

    if (!display.begin()) {
        LOG_E("DISPLAY", "ST7796 initialization failed");
        return false;
    }

    display.setRotation(BoardPins::DISPLAY_ROTATION);
    display.fillScreen(RGB565_BLACK);

    pinMode(BoardPins::LCD_BACKLIGHT, OUTPUT);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    backlightPwmReady = ledcAttach(
        BoardPins::LCD_BACKLIGHT,
        AppConfig::DISPLAY_BACKLIGHT_PWM_FREQUENCY_HZ,
        AppConfig::DISPLAY_BACKLIGHT_PWM_RESOLUTION_BITS);
#else
    constexpr std::uint8_t backlightChannel = 7;
    ledcSetup(
        backlightChannel,
        AppConfig::DISPLAY_BACKLIGHT_PWM_FREQUENCY_HZ,
        AppConfig::DISPLAY_BACKLIGHT_PWM_RESOLUTION_BITS);
    ledcAttachPin(BoardPins::LCD_BACKLIGHT, backlightChannel);
    backlightPwmReady = true;
#endif
    if (!backlightPwmReady) {
        digitalWrite(BoardPins::LCD_BACKLIGHT, HIGH);
        LOG_E("DISPLAY", "Backlight PWM initialization failed; using full brightness");
    } else {
        setBacklightPercent(100);
    }

    LOG_I("DISPLAY", "Ready: %d x %d on VSPI", display.width(), display.height());
    return true;
}

bool setBacklightPercent(std::uint8_t percent) {
    if (percent > 100U) {
        percent = 100U;
    }
    if (!backlightPwmReady) {
        digitalWrite(BoardPins::LCD_BACKLIGHT, percent == 0U ? LOW : HIGH);
        currentBacklightPercent = percent == 0U ? 0U : 100U;
        return false;
    }
    const std::uint32_t duty = dutyFromPercent(percent);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    if (!ledcWrite(BoardPins::LCD_BACKLIGHT, duty)) {
        return false;
    }
#else
    constexpr std::uint8_t backlightChannel = 7;
    ledcWrite(backlightChannel, duty);
#endif
    currentBacklightPercent = percent;
    return true;
}

std::uint8_t backlightPercent() {
    return currentBacklightPercent;
}

void drawRgb565Bitmap(
    std::int16_t x,
    std::int16_t y,
    std::uint16_t* pixels,
    std::uint16_t width,
    std::uint16_t height,
    bool byteSwapped) {

    if (byteSwapped) {
        display.draw16bitBeRGBBitmap(x, y, pixels, width, height);
    } else {
        display.draw16bitRGBBitmap(x, y, pixels, width, height);
    }
}

}  // namespace Display
}  // namespace Drivers
