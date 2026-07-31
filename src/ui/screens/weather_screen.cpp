#include "ui/screens/weather_screen.h"

#include <cstdio>
#include <lvgl.h>

#include "app/localization.h"
#include "services/geo_utils.h"
#include "ui/aprs_icons.h"
#include "ui/ui_components.h"

namespace Ui {
namespace WeatherScreen {
namespace {

lv_obj_t* listObject = nullptr;
lv_obj_t* countLabel = nullptr;
lv_obj_t* referenceLabel = nullptr;
std::uint32_t renderedRevision = 0xFFFFFFFFU;
std::uint32_t renderedReferenceRevision = 0xFFFFFFFFU;
std::size_t selected = 0;
const Services::WeatherStore::ViewState* currentState = nullptr;

void createMetricLabel(
    lv_obj_t* row,
    const char* text,
    lv_align_t alignment,
    lv_coord_t x,
    lv_coord_t y,
    std::uint32_t color) {

    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_align(label, alignment, x, y);
}

void createWeatherRow(
    const Services::WeatherStore::WeatherStation& station,
    bool isSelected,
    const Services::PositionReference& reference) {

    lv_obj_t* row = lv_obj_create(listObject);
    lv_obj_set_width(row, 438);
    lv_obj_set_height(row, 108);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(isSelected ? 0x56C7FF : 0x31425F), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_pad_left(row, 10, 0);
    lv_obj_set_style_pad_right(row, 10, 0);
    lv_obj_set_style_pad_top(row, 6, 0);
    lv_obj_set_style_pad_bottom(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* callLabel = lv_label_create(row);
    lv_label_set_text(callLabel, station.callsign);
    lv_obj_set_style_text_font(callLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(callLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(callLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* symbolIcon = AprsIcons::create(
        row,
        station.symbol[0],
        station.symbol[1],
        station.hasPosition);
    lv_obj_align(symbolIcon, LV_ALIGN_TOP_RIGHT, 0, -2);

    char relativeText[96] = {};
    std::snprintf(
        relativeText,
        sizeof(relativeText),
        "%s",
        App::Localization::text("Vzdalenost --", "Distance --"));
    if (station.hasPosition && reference.valid) {
        const Services::DistanceBearing relative = Services::calculateDistanceBearing(
            reference.latitude,
            reference.longitude,
            station.latitude,
            station.longitude);
        if (relative.valid) {
            std::snprintf(
                relativeText,
                sizeof(relativeText),
                "%.1f km %03.0f deg %s",
                relative.distanceKm,
                relative.bearingDegrees,
                Services::cardinalDirection(relative.bearingDegrees));
        }
    }
    lv_obj_t* relativeLabel = lv_label_create(row);
    lv_label_set_text(relativeLabel, relativeText);
    lv_obj_set_style_text_font(relativeLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(relativeLabel, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(relativeLabel, LV_ALIGN_TOP_RIGHT, -35, 3);

    char temperature[24];
    char humidity[24];
    char pressure[28];
    std::snprintf(temperature, sizeof(temperature), station.hasTemperature ? "%.1f C" : "--",
        static_cast<double>(station.temperatureC));
    std::snprintf(humidity, sizeof(humidity), station.hasHumidity ? "%.0f %%" : "--",
        static_cast<double>(station.humidityPercent));
    std::snprintf(pressure, sizeof(pressure), station.hasPressure ? "%.1f hPa" : "--",
        static_cast<double>(station.pressureHpa));
    char line1[160];
    std::snprintf(line1, sizeof(line1), "T %s   H %s   P %s", temperature, humidity, pressure);
    createMetricLabel(row, line1, LV_ALIGN_TOP_LEFT, 0, 27, 0xBDCAE0);

    char direction[20];
    char wind[24];
    char gust[24];
    std::snprintf(direction, sizeof(direction), station.hasWindDirection ? "%.0f deg" : "--",
        static_cast<double>(station.windDirectionDegrees));
    std::snprintf(wind, sizeof(wind), station.hasWindSpeed ? "%.1f km/h" : "--",
        static_cast<double>(station.windSpeedKmh));
    std::snprintf(gust, sizeof(gust), station.hasWindGust ? "%.1f" : "--",
        static_cast<double>(station.windGustKmh));
    char line2[160];
    std::snprintf(
        line2,
        sizeof(line2),
        App::Localization::text("V %s  %s   G %s km/h", "W %s  %s   G %s km/h"),
        direction,
        wind,
        gust);
    createMetricLabel(row, line2, LV_ALIGN_TOP_LEFT, 0, 47, 0xBDCAE0);

    char rain1h[20];
    char rain24h[20];
    char rainToday[20];
    std::snprintf(rain1h, sizeof(rain1h), station.hasRainLastHour ? "%.1f" : "--",
        static_cast<double>(station.rainLastHourMm));
    std::snprintf(rain24h, sizeof(rain24h), station.hasRainLast24Hours ? "%.1f" : "--",
        static_cast<double>(station.rainLast24HoursMm));
    std::snprintf(rainToday, sizeof(rainToday), station.hasRainToday ? "%.1f" : "--",
        static_cast<double>(station.rainTodayMm));
    char line3[160];
    if (station.hasSolarRadiation) {
        std::snprintf(
            line3,
            sizeof(line3),
            App::Localization::text(
                "Srazky 1h %s  24h %s  dnes %s mm | solar %.0f W/m2",
                "Rain 1h %s  24h %s  today %s mm | solar %.0f W/m2"),
            rain1h, rain24h, rainToday, static_cast<double>(station.solarRadiationWm2));
    } else {
        std::snprintf(
            line3,
            sizeof(line3),
            App::Localization::text(
                "Srazky 1h %s  24h %s  dnes %s mm",
                "Rain 1h %s  24h %s  today %s mm"),
            rain1h, rain24h, rainToday);
    }
    createMetricLabel(row, line3, LV_ALIGN_TOP_LEFT, 0, 67, 0x92A7C7);

    char positionText[120];
    if (station.hasPosition) {
        std::snprintf(
            positionText,
            sizeof(positionText),
            App::Localization::text("Pozice %.5f%c %.5f%c", "Position %.5f%c %.5f%c"),
            station.latitude < 0.0 ? -station.latitude : station.latitude,
            station.latitude < 0.0 ? 'S' : 'N',
            station.longitude < 0.0 ? -station.longitude : station.longitude,
            station.longitude < 0.0 ? 'W' : 'E');
    } else {
        std::snprintf(positionText, sizeof(positionText), App::Localization::text("Poloha v paketu neni", "No position in packet"));
    }
    createMetricLabel(row, positionText, LV_ALIGN_BOTTOM_LEFT, 0, 0, 0x92A7C7);
}

}  // namespace

void create() {
    resetScreen();
    createHeader(App::Localization::text("APRS meteostanice", "APRS weather stations"));

    referenceLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(referenceLabel, "Ref: --");
    lv_obj_set_style_text_font(referenceLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(referenceLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(referenceLabel, LV_ALIGN_TOP_RIGHT, -65, 17);

    countLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(countLabel, "0/5");
    lv_obj_set_style_text_font(countLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(countLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(countLabel, LV_ALIGN_TOP_RIGHT, -14, 16);

    listObject = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(listObject);
    lv_obj_set_size(listObject, 452, 194);
    lv_obj_align(listObject, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_pad_left(listObject, 5, 0);
    lv_obj_set_style_pad_right(listObject, 5, 0);
    lv_obj_set_style_pad_top(listObject, 2, 0);
    lv_obj_set_style_pad_bottom(listObject, 2, 0);
    lv_obj_set_style_pad_row(listObject, 5, 0);
    lv_obj_set_flex_flow(listObject, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(listObject, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(listObject, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(listObject, LV_SCROLLBAR_MODE_AUTO);
    renderedRevision = 0xFFFFFFFFU;
    renderedReferenceRevision = 0xFFFFFFFFU;
    selected = 0;
}

void update(
    const Services::WeatherStore::ViewState& state,
    const Services::PositionReference& reference) {

    if (listObject == nullptr || countLabel == nullptr || referenceLabel == nullptr) {
        return;
    }
    if (renderedRevision == state.revision &&
        renderedReferenceRevision == reference.revision) {
        return;
    }
    currentState = &state;
    if (state.count == 0) selected = 0; else if (selected >= state.count) selected = state.count - 1;
    renderedRevision = state.revision;
    renderedReferenceRevision = reference.revision;
    lv_obj_clean(listObject);
    lv_label_set_text(referenceLabel, reference.valid ? (reference.fromGps ? "Ref: GPS" : "Ref: DEF") : "Ref: --");

    char countText[16];
    std::snprintf(countText, sizeof(countText), "%u/%u",
        static_cast<unsigned>(state.count),
        static_cast<unsigned>(Services::WeatherStore::MAX_STATIONS));
    lv_label_set_text(countLabel, countText);

    if (state.count == 0) {
        lv_obj_t* emptyLabel = lv_label_create(listObject);
        lv_label_set_text(
            emptyLabel,
            App::Localization::text(
                "Zatim nebyl prijat platny APRS meteorologicky paket.",
                "No valid APRS weather packet has been received yet."));
        lv_obj_set_width(emptyLabel, 420);
        lv_label_set_long_mode(emptyLabel, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(emptyLabel, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(emptyLabel, lv_color_hex(0x92A7C7), 0);
        return;
    }

    for (std::size_t index = 0; index < state.count; ++index) {
        createWeatherRow(state.stations[index], index == selected, reference);
    }
    lv_obj_scroll_to_y(listObject, 0, LV_ANIM_OFF);
}

void moveSelection(int direction) {
    if (currentState == nullptr || currentState->count == 0 || direction == 0) return;
    if (direction < 0) selected = selected == 0 ? currentState->count - 1 : selected - 1;
    else selected = (selected + 1) % currentState->count;
    renderedRevision = 0xFFFFFFFFU;
}

std::size_t selectedIndex() { return selected; }


}  // namespace WeatherScreen
}  // namespace Ui
