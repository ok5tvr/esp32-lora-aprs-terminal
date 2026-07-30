#include "drivers/sd_card_driver.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "app_config.h"
#include "app_log.h"
#include "board_pins.h"

namespace Drivers {
namespace SdCard {
namespace {

Status currentStatus;

const char* cardTypeName(std::uint8_t type) {
    switch (type) {
        case CARD_MMC:
            return "MMC";
        case CARD_SD:
            return "SDSC";
        case CARD_SDHC:
            return "SDHC";
        default:
            return "UNKNOWN";
    }
}

}  // namespace

bool begin() {
    currentStatus = Status{};

    pinMode(BoardPins::LCD_CS, OUTPUT);
    digitalWrite(BoardPins::LCD_CS, HIGH);
    pinMode(BoardPins::SD_CS, OUTPUT);
    digitalWrite(BoardPins::SD_CS, HIGH);

    // The onboard SD slot shares VSPI data lines with the LCD, but has its own CS.
    SPI.begin(
        BoardPins::SD_SCLK,
        BoardPins::SD_MISO,
        BoardPins::SD_MOSI,
        BoardPins::SD_CS);

    if (!SD.begin(BoardPins::SD_CS, SPI, AppConfig::SD_SPI_FREQUENCY_HZ)) {
        LOG_E("SD", "Card mount failed; display and LoRa remain available");
        return false;
    }

    currentStatus.cardType = SD.cardType();
    if (currentStatus.cardType == CARD_NONE) {
        LOG_E("SD", "No card inserted");
        SD.end();
        return false;
    }

    currentStatus.mounted = true;
    currentStatus.cardSizeBytes = SD.cardSize();
    refreshUsage();

    LOG_I(
        "SD",
        "%s card ready, size %llu MB, used %llu MB",
        cardTypeName(currentStatus.cardType),
        static_cast<unsigned long long>(currentStatus.cardSizeBytes / (1024ULL * 1024ULL)),
        static_cast<unsigned long long>(currentStatus.usedBytes / (1024ULL * 1024ULL)));
    return true;
}

const Status& status() {
    return currentStatus;
}

void refreshUsage() {
    if (!currentStatus.mounted) {
        return;
    }
    currentStatus.totalBytes = SD.totalBytes();
    currentStatus.usedBytes = SD.usedBytes();
}

}  // namespace SdCard
}  // namespace Drivers
