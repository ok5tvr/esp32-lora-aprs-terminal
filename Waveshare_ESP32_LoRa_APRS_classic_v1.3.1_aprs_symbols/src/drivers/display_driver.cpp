#include "drivers/display_driver.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <TCA9554.h>
#include <Wire.h>

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
    digitalWrite(BoardPins::LCD_BACKLIGHT, HIGH);

    LOG_I("DISPLAY", "Ready: %d x %d on VSPI", display.width(), display.height());
    return true;
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
