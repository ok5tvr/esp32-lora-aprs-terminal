#include "ui/screens/weather_detail_screen.h"

#include <cstdio>
#include <lvgl.h>

#include "app/localization.h"
#include "services/geo_utils.h"
#include "ui/aprs_icons.h"
#include "ui/ui_components.h"

namespace Ui {
namespace WeatherDetailScreen {
namespace {

lv_obj_t* titleLabel = nullptr;
lv_obj_t* mainLabel = nullptr;
lv_obj_t* extraLabel = nullptr;
lv_obj_t* packetLabel = nullptr;
lv_obj_t* iconObject = nullptr;

void ageText(char* output, std::size_t capacity, std::uint32_t milliseconds) {
    const std::uint32_t seconds = milliseconds / 1000U;
    if (seconds < 60U) {
        std::snprintf(output, capacity, "%lu s", static_cast<unsigned long>(seconds));
    } else if (seconds < 3600U) {
        std::snprintf(output, capacity, "%lu min", static_cast<unsigned long>(seconds / 60U));
    } else {
        std::snprintf(
            output,
            capacity,
            "%lu h %lu min",
            static_cast<unsigned long>(seconds / 3600U),
            static_cast<unsigned long>((seconds / 60U) % 60U));
    }
}

}  // namespace

void create() {
    resetScreen();
    createHeader(App::Localization::text("Detail meteostanice", "Weather station details"));

    lv_obj_t* card = lv_obj_create(lv_scr_act());
    lv_obj_set_size(card, 450, 194);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    titleLabel = lv_label_create(card);
    lv_obj_set_width(titleLabel, 360);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    mainLabel = lv_label_create(card);
    lv_obj_set_width(mainLabel, 425);
    lv_label_set_long_mode(mainLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(mainLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(mainLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(mainLabel, LV_ALIGN_TOP_LEFT, 0, 34);

    extraLabel = lv_label_create(card);
    lv_obj_set_width(extraLabel, 425);
    lv_label_set_long_mode(extraLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(extraLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(extraLabel, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(extraLabel, LV_ALIGN_TOP_LEFT, 0, 100);

    packetLabel = lv_label_create(card);
    lv_obj_set_width(packetLabel, 425);
    lv_obj_set_height(packetLabel, 42);
    lv_label_set_long_mode(packetLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(packetLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(packetLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(packetLabel, LV_ALIGN_TOP_LEFT, 0, 136);
}

void update(
    const Services::WeatherStore::WeatherStation& station,
    const Services::PositionReference& reference,
    std::uint32_t now) {

    if (titleLabel == nullptr) {
        return;
    }

    lv_label_set_text(
        titleLabel,
        station.entityName[0] != '\0' ? station.entityName : station.callsign);
    if (iconObject == nullptr) {
        iconObject = AprsIcons::create(
            lv_obj_get_parent(titleLabel),
            station.symbol[0],
            station.symbol[1],
            station.hasPosition);
        lv_obj_align(iconObject, LV_ALIGN_TOP_RIGHT, -2, 4);
    }

    char temperature[20] = "--";
    char humidity[20] = "--";
    char pressure[24] = "--";
    if (station.hasTemperature) {
        std::snprintf(temperature, sizeof(temperature), "%.1f C", static_cast<double>(station.temperatureC));
    }
    if (station.hasHumidity) {
        std::snprintf(humidity, sizeof(humidity), "%.0f %%", static_cast<double>(station.humidityPercent));
    }
    if (station.hasPressure) {
        std::snprintf(pressure, sizeof(pressure), "%.1f hPa", static_cast<double>(station.pressureHpa));
    }

    lv_label_set_text_fmt(
        mainLabel,
        App::Localization::text(
            "Teplota %s | vlhkost %s\nTlak %s\nVitr %.0f deg / %.1f km/h | naraz %.1f km/h",
            "Temperature %s | humidity %s\nPressure %s\nWind %.0f deg / %.1f km/h | gust %.1f km/h"),
        temperature,
        humidity,
        pressure,
        static_cast<double>(station.windDirectionDegrees),
        static_cast<double>(station.windSpeedKmh),
        static_cast<double>(station.windGustKmh));

    char age[24];
    ageText(age, sizeof(age), now - station.lastHeardMs);
    char relative[80];
    std::snprintf(
        relative,
        sizeof(relative),
        "%s",
        App::Localization::text("vzdalenost --", "distance --"));
    if (station.hasPosition && reference.valid) {
        const Services::DistanceBearing result = Services::calculateDistanceBearing(
            reference.latitude,
            reference.longitude,
            station.latitude,
            station.longitude);
        if (result.valid) {
            std::snprintf(
                relative,
                sizeof(relative),
                "%.2f km | %03.0f deg %s",
                result.distanceKm,
                result.bearingDegrees,
                Services::cardinalDirection(result.bearingDegrees));
        }
    }

    lv_label_set_text_fmt(
        extraLabel,
        App::Localization::text(
            "Srazky 1h %.1f | 24h %.1f | dnes %.1f mm | solar %.0f W/m2\n%s | RSSI %.1f | SNR %.1f | stari %s",
            "Rain 1h %.1f | 24h %.1f | today %.1f mm | solar %.0f W/m2\n%s | RSSI %.1f | SNR %.1f | age %s"),
        static_cast<double>(station.rainLastHourMm),
        static_cast<double>(station.rainLast24HoursMm),
        static_cast<double>(station.rainTodayMm),
        static_cast<double>(station.solarRadiationWm2),
        relative,
        static_cast<double>(station.lastRssiDbm),
        static_cast<double>(station.lastSnrDb),
        age);

    lv_label_set_text(
        packetLabel,
        station.lastFrame[0] != '\0'
            ? station.lastFrame
            : App::Localization::text(
                "TNC2 ramec neni ulozen.",
                "The TNC2 frame is not stored."));
}

}  // namespace WeatherDetailScreen
}  // namespace Ui
