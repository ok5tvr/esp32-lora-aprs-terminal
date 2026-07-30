#include "ui/screens/gps_screen.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <lvgl.h>

#include "app_config.h"
#include "board_pins.h"
#include "services/geo_utils.h"
#include "ui/ui_components.h"

namespace Ui {
namespace GpsScreen {
namespace {

lv_obj_t* stateLabel = nullptr;
lv_obj_t* packetLabel = nullptr;
lv_obj_t* positionLabel = nullptr;
lv_obj_t* locatorLabel = nullptr;
lv_obj_t* movementLabel = nullptr;
lv_obj_t* qualityLabel = nullptr;
lv_obj_t* sentenceLabel = nullptr;
std::uint32_t renderedRevision = 0xFFFFFFFFU;

void styleValue(lv_obj_t* label, lv_coord_t y, lv_color_t color = lv_color_hex(0xBDCAE0)) {
    lv_obj_set_width(label, 452);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 14, y);
}

void formatAge(char* output, std::size_t capacity, std::uint32_t ageMs) {
    if (ageMs == 0xFFFFFFFFU) {
        std::snprintf(output, capacity, "--");
    } else if (ageMs < 1000U) {
        std::snprintf(output, capacity, "%u ms", static_cast<unsigned>(ageMs));
    } else {
        std::snprintf(output, capacity, "%.1f s", static_cast<double>(ageMs) / 1000.0);
    }
}

}  // namespace

void create() {
    resetScreen();
    createHeader("GPS diagnostika");

    stateLabel = lv_label_create(lv_scr_act());
    styleValue(stateLabel, 56);
    lv_obj_set_style_text_font(stateLabel, &lv_font_montserrat_18, 0);

    packetLabel = lv_label_create(lv_scr_act());
    styleValue(packetLabel, 84, lv_color_hex(0x92A7C7));

    positionLabel = lv_label_create(lv_scr_act());
    styleValue(positionLabel, 111);

    locatorLabel = lv_label_create(lv_scr_act());
    styleValue(locatorLabel, 138, lv_color_hex(0x56C7FF));

    movementLabel = lv_label_create(lv_scr_act());
    styleValue(movementLabel, 165);

    qualityLabel = lv_label_create(lv_scr_act());
    styleValue(qualityLabel, 192);

    sentenceLabel = lv_label_create(lv_scr_act());
    lv_obj_set_size(sentenceLabel, 452, 42);
    lv_label_set_long_mode(sentenceLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(sentenceLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sentenceLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(sentenceLabel, LV_ALIGN_TOP_LEFT, 14, 219);

    renderedRevision = 0xFFFFFFFFU;
}

void update(const Services::GpsService::ViewState& state) {
    if (stateLabel == nullptr || renderedRevision == state.revision) {
        return;
    }
    renderedRevision = state.revision;

    if (!state.uartStarted) {
        lv_label_set_text(stateLabel, "GPS UART je vypnut");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xFFB454), 0);
    } else if (!state.serialTrafficDetected) {
        lv_label_set_text(stateLabel, "GPS bez dat - cekam na UART");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xFFB454), 0);
    } else if (!state.nmeaPacketDetected) {
        lv_label_set_text(stateLabel, "GPS data prijata - cekam na NMEA vetu");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xFFB454), 0);
    } else if (!state.receiverDetected) {
        lv_label_set_text(stateLabel, "NMEA veta prijata - bez platneho checksumu");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xFF7C7C), 0);
    } else if (!state.hasFix) {
        lv_label_set_text(stateLabel, "GPS nalezena - cekam na fix");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x56C7FF), 0);
    } else {
        lv_label_set_text(stateLabel, "GPS fix je platny");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x42D392), 0);
    }

    char sentenceAge[20];
    char validAge[20];
    formatAge(sentenceAge, sizeof(sentenceAge), state.lastSentenceAgeMs);
    formatAge(validAge, sizeof(validAge), state.lastValidPacketAgeMs);

    char text[192];
    std::snprintf(
        text,
        sizeof(text),
        "UART2 RX GPIO%d | %u Bd | NMEA %s | veta %s | valid %s",
        BoardPins::GPS_RX,
        static_cast<unsigned>(AppConfig::GPS_BAUD_RATE),
        state.lastSentenceType,
        sentenceAge,
        validAge);
    lv_label_set_text(packetLabel, text);

    if (state.hasFix) {
        const char latitudeHemisphere = state.latitude >= 0.0 ? 'N' : 'S';
        const char longitudeHemisphere = state.longitude >= 0.0 ? 'E' : 'W';
        std::snprintf(
            text,
            sizeof(text),
            "Pozice: %.6f %c | %.6f %c | vyska %.1f m",
            std::fabs(state.latitude),
            latitudeHemisphere,
            std::fabs(state.longitude),
            longitudeHemisphere,
            state.altitudeMeters);
    } else {
        std::snprintf(text, sizeof(text), "Pozice: --");
    }
    lv_label_set_text(positionLabel, text);

    char fixAge[20];
    formatAge(fixAge, sizeof(fixAge), state.fixAgeMs);
    std::snprintf(
        text,
        sizeof(text),
        "Lokator: %s | stari fixu: %s",
        state.locatorValid ? state.locator : "------",
        fixAge);
    lv_label_set_text(locatorLabel, text);

    if (state.speedValid && state.courseValid) {
        std::snprintf(
            text,
            sizeof(text),
            "Rychlost: %.1f km/h | smer: %03.0f deg %s",
            static_cast<double>(state.speedKmh),
            static_cast<double>(state.courseDegrees),
            Services::cardinalDirection(state.courseDegrees));
    } else if (state.speedValid) {
        std::snprintf(
            text,
            sizeof(text),
            "Rychlost: %.1f km/h | smer: --",
            static_cast<double>(state.speedKmh));
    } else {
        std::snprintf(text, sizeof(text), "Rychlost: -- | smer: --");
    }
    lv_label_set_text(movementLabel, text);

    if (state.utcTimeValid && state.utcDateValid) {
        std::snprintf(
            text,
            sizeof(text),
            "Satelity: %u | HDOP: %.1f | UTC %02u:%02u:%02u %02u.%02u.%04u",
            static_cast<unsigned>(state.satellites),
            static_cast<double>(state.hdop),
            static_cast<unsigned>(state.utcHour),
            static_cast<unsigned>(state.utcMinute),
            static_cast<unsigned>(state.utcSecond),
            static_cast<unsigned>(state.utcDay),
            static_cast<unsigned>(state.utcMonth),
            static_cast<unsigned>(state.utcYear));
    } else if (state.utcTimeValid) {
        std::snprintf(
            text,
            sizeof(text),
            "Satelity: %u | HDOP: %.1f | UTC %02u:%02u:%02u",
            static_cast<unsigned>(state.satellites),
            static_cast<double>(state.hdop),
            static_cast<unsigned>(state.utcHour),
            static_cast<unsigned>(state.utcMinute),
            static_cast<unsigned>(state.utcSecond));
    } else {
        std::snprintf(
            text,
            sizeof(text),
            "Satelity: %u | HDOP: %.1f | UTC --",
            static_cast<unsigned>(state.satellites),
            static_cast<double>(state.hdop));
    }
    lv_label_set_text(qualityLabel, text);

    if (state.lastNmeaSentence[0] == '$') {
        lv_label_set_text(sentenceLabel, state.lastNmeaSentence);
    } else {
        lv_label_set_text(sentenceLabel, "Cekam na prvni NMEA vetu...");
    }
}

}  // namespace GpsScreen
}  // namespace Ui
