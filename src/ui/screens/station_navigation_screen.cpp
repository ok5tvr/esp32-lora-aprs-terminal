#include "ui/screens/station_navigation_screen.h"

#include <cstdio>
#include <lvgl.h>

#include "services/geo_utils.h"
#include "ui/ui_components.h"

namespace Ui {
namespace StationNavigationScreen {
namespace {

lv_obj_t* targetLabel = nullptr;
lv_obj_t* bearingLabel = nullptr;
lv_obj_t* distanceLabel = nullptr;
lv_obj_t* statusLabel = nullptr;
lv_obj_t* coordinatesLabel = nullptr;

}  // namespace

void create() {
    resetScreen();
    createHeader("Navigace k APRS stanici");

    targetLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(targetLabel, 440);
    lv_label_set_long_mode(targetLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(targetLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(targetLabel, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(targetLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(targetLabel, LV_ALIGN_TOP_MID, 0, 58);

    bearingLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(bearingLabel, 440);
    lv_obj_set_style_text_align(bearingLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(bearingLabel, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(bearingLabel, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(bearingLabel, LV_ALIGN_TOP_MID, 0, 92);

    distanceLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(distanceLabel, 440);
    lv_obj_set_style_text_align(distanceLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(distanceLabel, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(distanceLabel, lv_color_hex(0x42D392), 0);
    lv_obj_align(distanceLabel, LV_ALIGN_TOP_MID, 0, 136);

    statusLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(statusLabel, 440);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 174);

    coordinatesLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(coordinatesLabel, 440);
    lv_obj_set_style_text_align(coordinatesLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(coordinatesLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(coordinatesLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(coordinatesLabel, LV_ALIGN_TOP_MID, 0, 217);
}

void update(
    const Services::StationStore::Station& station,
    const Services::PositionReference& reference,
    std::uint32_t now) {

    if (targetLabel == nullptr) {
        return;
    }
    const char* target = station.type == Aprs::EntityType::Station
        ? station.callsign
        : station.entityName;
    lv_label_set_text(targetLabel, target);

    if (!station.hasPosition || !reference.valid) {
        lv_label_set_text(bearingLabel, "SMER --");
        lv_label_set_text(distanceLabel, "Vzdalenost --");
        lv_label_set_text(statusLabel, !station.hasPosition
            ? "Cil nema platnou polohu."
            : "Neni dostupna referencni poloha terminalu.");
        lv_label_set_text(coordinatesLabel, "--");
        return;
    }

    const Services::DistanceBearing relative = Services::calculateDistanceBearing(
        reference.latitude, reference.longitude, station.latitude, station.longitude);
    if (!relative.valid) {
        lv_label_set_text(bearingLabel, "SMER --");
        lv_label_set_text(distanceLabel, "Vzdalenost --");
        lv_label_set_text(statusLabel, "Vypocet navigace selhal.");
        return;
    }

    lv_label_set_text_fmt(
        bearingLabel,
        "%03.0f deg  %s",
        relative.bearingDegrees,
        Services::cardinalDirection(relative.bearingDegrees));
    if (relative.distanceKm < 1.0) {
        lv_label_set_text_fmt(distanceLabel, "%.0f m", relative.distanceKm * 1000.0);
    } else {
        lv_label_set_text_fmt(distanceLabel, "%.2f km", relative.distanceKm);
    }

    const std::uint32_t ageSeconds = (now - station.lastHeardMs) / 1000U;
    lv_label_set_text_fmt(
        statusLabel,
        "Reference: %s | pozice cile stara %lu s\nSmer je zemepisny azimut, nikoli kompasovy kurz pristroje.",
        reference.fromGps ? "aktualni GPS" : "vychozi poloha",
        static_cast<unsigned long>(ageSeconds));
    lv_label_set_text_fmt(
        coordinatesLabel,
        "Cil %.5f, %.5f | terminal %.5f, %.5f",
        station.latitude,
        station.longitude,
        reference.latitude,
        reference.longitude);
}

}  // namespace StationNavigationScreen
}  // namespace Ui
