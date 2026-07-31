#include "ui/screens/astronomy_screen.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "app/localization.h"
#include "ui/ui_components.h"

namespace Ui {
namespace AstronomyScreen {
namespace {

constexpr std::uint16_t MOON_CANVAS_SIZE = 68U;
constexpr double ASTRO_PI = 3.14159265358979323846;
constexpr double ASTRO_DEG_TO_RAD = ASTRO_PI / 180.0;

lv_obj_t* dateLabel = nullptr;
lv_obj_t* positionLabel = nullptr;
lv_obj_t* sunTitleLabel = nullptr;
lv_obj_t* sunTimesLabel = nullptr;
lv_obj_t* daylightLabel = nullptr;
lv_obj_t* moonTitleLabel = nullptr;
lv_obj_t* moonTimesLabel = nullptr;
lv_obj_t* phaseLabel = nullptr;
lv_obj_t* moonCanvas = nullptr;
lv_color_t moonCanvasBuffer[MOON_CANVAS_SIZE * MOON_CANVAS_SIZE];
std::uint32_t renderedRevision = 0xFFFFFFFFU;
Services::TimeService::Source renderedTimeSource = Services::TimeService::Source::None;

void styleLine(
    lv_obj_t* label,
    lv_coord_t y,
    const lv_font_t* font,
    lv_color_t color = lv_color_hex(0xBDCAE0),
    lv_coord_t width = 452) {

    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 14, y);
}

void formatCoordinate(char* output, std::size_t capacity, double value) {
    std::snprintf(output, capacity, "%.4f", std::fabs(value));
    if (!App::Localization::isEnglish()) {
        if (char* decimal = std::strchr(output, '.')) {
            *decimal = ',';
        }
    }
}

void formatEvent(
    char* output,
    std::size_t capacity,
    const Services::AstronomyService::EventTime& event) {

    if (event.valid) {
        std::snprintf(
            output,
            capacity,
            "%02u:%02u",
            static_cast<unsigned>(event.hour),
            static_cast<unsigned>(event.minute));
    } else {
        std::snprintf(output, capacity, "--:--");
    }
}

void drawMoon(double elongationDegrees) {
    if (moonCanvas == nullptr) {
        return;
    }

    const lv_color_t background = lv_color_hex(0x0B1424);
    const lv_color_t dark = lv_color_hex(0x17233A);
    const lv_color_t edge = lv_color_hex(0x67748A);
    const lv_color_t litLow = lv_color_hex(0xD8D1B3);
    const lv_color_t litHigh = lv_color_hex(0xFFF5C9);
    const double center = (static_cast<double>(MOON_CANVAS_SIZE) - 1.0) * 0.5;
    const double radius = center - 2.0;
    const double elongation = elongationDegrees * ASTRO_DEG_TO_RAD;
    const double sunX = std::sin(elongation);
    const double sunZ = -std::cos(elongation);

    for (std::uint16_t y = 0; y < MOON_CANVAS_SIZE; ++y) {
        for (std::uint16_t x = 0; x < MOON_CANVAS_SIZE; ++x) {
            const double nx = (static_cast<double>(x) - center) / radius;
            const double ny = (center - static_cast<double>(y)) / radius;
            const double radialSquared = nx * nx + ny * ny;
            lv_color_t color = background;

            if (radialSquared <= 1.0) {
                const double nz = std::sqrt(std::max(0.0, 1.0 - radialSquared));
                const double illuminationDot = nx * sunX + nz * sunZ;
                if (radialSquared > 0.91) {
                    color = edge;
                } else if (illuminationDot > 0.0) {
                    color = nz > 0.55 ? litHigh : litLow;
                } else {
                    color = dark;
                }
            }
            lv_canvas_set_px_color(moonCanvas, x, y, color);
        }
    }
    lv_obj_invalidate(moonCanvas);
}

void setAltitudeText(
    char* output,
    std::size_t capacity,
    const Services::AstronomyService::ViewState& state) {

    if (state.daylightMinutes > 0U) {
        std::snprintf(
            output,
            capacity,
            App::Localization::text(
                "Delka dne: %u h %02u min | vyska Slunce: %+.1f st.",
                "Daylight: %u h %02u min | Sun altitude: %+.1f deg"),
            static_cast<unsigned>(state.daylightMinutes / 60U),
            static_cast<unsigned>(state.daylightMinutes % 60U),
            static_cast<double>(state.sunAltitudeDegrees));
    } else {
        std::snprintf(
            output,
            capacity,
            App::Localization::text(
                "Vyska Slunce: %+.1f st.",
                "Sun altitude: %+.1f deg"),
            static_cast<double>(state.sunAltitudeDegrees));
    }
    if (!App::Localization::isEnglish()) {
        if (char* decimal = std::strrchr(output, '.')) {
            *decimal = ',';
        }
    }
}

}  // namespace

void create() {
    resetScreen();
    createHeader(App::Localization::text("Astronomie", "Astronomy"));

    dateLabel = lv_label_create(lv_scr_act());
    styleLine(dateLabel, 54, &lv_font_montserrat_14, lv_color_hex(0x92A7C7));

    positionLabel = lv_label_create(lv_scr_act());
    styleLine(positionLabel, 75, &lv_font_montserrat_14, lv_color_hex(0x92A7C7));

    sunTitleLabel = lv_label_create(lv_scr_act());
    styleLine(sunTitleLabel, 98, &lv_font_montserrat_18, lv_color_hex(0xFFD166));
    lv_label_set_text(sunTitleLabel, App::Localization::text("Slunce", "Sun"));

    sunTimesLabel = lv_label_create(lv_scr_act());
    styleLine(sunTimesLabel, 123, &lv_font_montserrat_16, lv_color_hex(0xF4F7FF));

    daylightLabel = lv_label_create(lv_scr_act());
    styleLine(daylightLabel, 148, &lv_font_montserrat_14, lv_color_hex(0xBDCAE0));

    moonTitleLabel = lv_label_create(lv_scr_act());
    styleLine(moonTitleLabel, 174, &lv_font_montserrat_18, lv_color_hex(0xB8C8FF), 370);
    lv_label_set_text(moonTitleLabel, App::Localization::text("Mesic", "Moon"));

    moonTimesLabel = lv_label_create(lv_scr_act());
    styleLine(moonTimesLabel, 199, &lv_font_montserrat_16, lv_color_hex(0xF4F7FF), 370);

    phaseLabel = lv_label_create(lv_scr_act());
    styleLine(phaseLabel, 224, &lv_font_montserrat_14, lv_color_hex(0xBDCAE0), 370);

    moonCanvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(
        moonCanvas,
        moonCanvasBuffer,
        MOON_CANVAS_SIZE,
        MOON_CANVAS_SIZE,
        LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(moonCanvas, MOON_CANVAS_SIZE, MOON_CANVAS_SIZE);
    lv_obj_align(moonCanvas, LV_ALIGN_TOP_RIGHT, -12, 174);
    drawMoon(0.0);

    renderedRevision = 0xFFFFFFFFU;
    renderedTimeSource = Services::TimeService::Source::None;
}

void update(
    const Services::AstronomyService::ViewState& state,
    const Services::TimeService::ViewState& timeState) {

    if (dateLabel == nullptr ||
        (renderedRevision == state.revision && renderedTimeSource == timeState.source)) {
        return;
    }
    renderedRevision = state.revision;
    renderedTimeSource = timeState.source;

    if (!state.valid) {
        lv_label_set_text(
            dateLabel,
            App::Localization::text(
                "Cekam na platny cas a polohu.",
                "Waiting for valid time and position."));
        lv_label_set_text(positionLabel, App::Localization::text("Poloha: --", "Position: --"));
        lv_label_set_text(sunTimesLabel, App::Localization::text("Vychod: --:-- | Zapad: --:--", "Rise: --:-- | Set: --:--"));
        lv_label_set_text(daylightLabel, App::Localization::text("Vyska Slunce: --", "Sun altitude: --"));
        lv_label_set_text(moonTimesLabel, App::Localization::text("Vychod: --:-- | Zapad: --:--", "Rise: --:-- | Set: --:--"));
        lv_label_set_text(phaseLabel, App::Localization::text("Faze: -- | osvetleni: --", "Phase: -- | illumination: --"));
        drawMoon(0.0);
        return;
    }

    const char* timeSource = Services::timeSourceText(timeState.source);
    if (App::Localization::isEnglish()) {
        lv_label_set_text_fmt(
            dateLabel,
            "%04u-%02u-%02u | time: %s",
            static_cast<unsigned>(state.year),
            static_cast<unsigned>(state.month),
            static_cast<unsigned>(state.day),
            timeSource);
    } else {
        lv_label_set_text_fmt(
            dateLabel,
            "%02u.%02u.%04u | cas: %s",
            static_cast<unsigned>(state.day),
            static_cast<unsigned>(state.month),
            static_cast<unsigned>(state.year),
            timeSource);
    }

    char latitude[20];
    char longitude[20];
    formatCoordinate(latitude, sizeof(latitude), state.latitude);
    formatCoordinate(longitude, sizeof(longitude), state.longitude);
    lv_label_set_text_fmt(
        positionLabel,
        App::Localization::text("Poloha %s | %s %c | %s %c", "Position %s | %s %c | %s %c"),
        state.positionFromGps ? "GPS" : App::Localization::text("vychozi", "default"),
        latitude,
        state.latitude >= 0.0 ? 'N' : 'S',
        longitude,
        state.longitude >= 0.0 ? 'E' : 'W');

    char rise[12];
    char set[12];
    formatEvent(rise, sizeof(rise), state.sunrise);
    formatEvent(set, sizeof(set), state.sunset);
    if (state.sunAboveAllDay) {
        lv_label_set_text(sunTimesLabel, App::Localization::text("Cely den nad obzorem", "Above horizon all day"));
    } else if (state.sunBelowAllDay) {
        lv_label_set_text(sunTimesLabel, App::Localization::text("Cely den pod obzorem", "Below horizon all day"));
    } else {
        lv_label_set_text_fmt(
            sunTimesLabel,
            App::Localization::text("Vychod: %s | zapad: %s", "Rise: %s | set: %s"),
            rise,
            set);
    }

    char altitudeText[160];
    setAltitudeText(altitudeText, sizeof(altitudeText), state);
    lv_label_set_text(daylightLabel, altitudeText);

    formatEvent(rise, sizeof(rise), state.moonrise);
    formatEvent(set, sizeof(set), state.moonset);
    if (state.moonAboveAllDay) {
        lv_label_set_text(moonTimesLabel, App::Localization::text("Cely den nad obzorem", "Above horizon all day"));
    } else if (state.moonBelowAllDay) {
        lv_label_set_text(moonTimesLabel, App::Localization::text("Cely den pod obzorem", "Below horizon all day"));
    } else {
        lv_label_set_text_fmt(
            moonTimesLabel,
            App::Localization::text("Vychod: %s | zapad: %s", "Rise: %s | set: %s"),
            rise,
            set);
    }

    const char* phase = App::Localization::isEnglish()
        ? Services::moonPhaseTextEnglish(state.moonPhase)
        : Services::moonPhaseTextCzech(state.moonPhase);
    char phaseText[192];
    std::snprintf(
        phaseText,
        sizeof(phaseText),
        App::Localization::text(
            "Faze: %s | osvetleni: %u %% | stari: %.1f dne",
            "Phase: %s | illumination: %u %% | age: %.1f d"),
        phase,
        static_cast<unsigned>(state.moonIlluminationPercent),
        static_cast<double>(state.moonAgeDays));
    if (!App::Localization::isEnglish()) {
        if (char* decimal = std::strrchr(phaseText, '.')) {
            *decimal = ',';
        }
    }
    lv_label_set_text(phaseLabel, phaseText);
    drawMoon(state.moonElongationDegrees);
}

}  // namespace AstronomyScreen
}  // namespace Ui
