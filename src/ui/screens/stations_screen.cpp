#include "ui/screens/stations_screen.h"

#include <cstdio>
#include <lvgl.h>

#include "app/localization.h"
#include "services/geo_utils.h"
#include "ui/aprs_icons.h"
#include "ui/ui_components.h"

namespace Ui {
namespace StationsScreen {
namespace {

lv_obj_t* listObject = nullptr;
lv_obj_t* countLabel = nullptr;
lv_obj_t* referenceLabel = nullptr;
lv_obj_t* renderedRows[Services::StationStore::MAX_STATIONS] = {};
std::uint32_t renderedRevision = 0xFFFFFFFFU;
std::uint32_t renderedReferenceRevision = 0xFFFFFFFFU;
std::size_t selected = 0;
std::size_t currentCount = 0;

void styleRow(lv_obj_t* row, bool selectedRow) {
    lv_obj_set_width(row, 438);
    lv_obj_set_height(row, 62);
    lv_obj_set_style_bg_color(row, lv_color_hex(selectedRow ? 0x203A58 : 0x17243A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(selectedRow ? 0x56C7FF : 0x31425F), 0);
    lv_obj_set_style_border_width(row, selectedRow ? 2 : 1, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_pad_left(row, 12, 0);
    lv_obj_set_style_pad_right(row, 12, 0);
    lv_obj_set_style_pad_top(row, 6, 0);
    lv_obj_set_style_pad_bottom(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* createStationRow(
    const Services::StationStore::Station& station,
    const Services::PositionReference& reference,
    bool selectedRow) {

    lv_obj_t* row = lv_obj_create(listObject);
    styleRow(row, selectedRow);

    char titleText[72];
    if (station.type == Aprs::EntityType::Object) {
        std::snprintf(titleText, sizeof(titleText), "OBJ:%s  <%s>", station.entityName, station.callsign);
    } else if (station.type == Aprs::EntityType::Item) {
        std::snprintf(titleText, sizeof(titleText), "ITEM:%s  <%s>", station.entityName, station.callsign);
    } else {
        std::snprintf(titleText, sizeof(titleText), "%s", station.callsign);
    }

    lv_obj_t* callLabel = lv_label_create(row);
    lv_label_set_text(callLabel, titleText);
    lv_obj_set_width(callLabel, 350);
    lv_label_set_long_mode(callLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(callLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(callLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(callLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* symbolIcon = AprsIcons::create(
        row, station.symbol[0], station.symbol[1], station.hasPosition);
    lv_obj_align(symbolIcon, LV_ALIGN_TOP_RIGHT, 0, -2);

    lv_obj_t* positionLabel = lv_label_create(row);
    char positionText[160];
    if (station.hasPosition) {
        const Services::DistanceBearing relative = reference.valid
            ? Services::calculateDistanceBearing(
                reference.latitude, reference.longitude, station.latitude, station.longitude)
            : Services::DistanceBearing{};
        if (relative.valid) {
            std::snprintf(
                positionText,
                sizeof(positionText),
                "%.5f, %.5f | %.1f km %03.0f deg %s",
                station.latitude,
                station.longitude,
                relative.distanceKm,
                relative.bearingDegrees,
                Services::cardinalDirection(relative.bearingDegrees));
        } else {
            std::snprintf(positionText, sizeof(positionText), App::Localization::text("%.5f, %.5f | vzdalenost --", "%.5f, %.5f | distance --"), station.latitude, station.longitude);
        }
    } else {
        std::snprintf(positionText, sizeof(positionText), App::Localization::text("Poloha v paketu neni", "No position in packet"));
    }
    lv_label_set_text(positionLabel, positionText);
    lv_obj_set_width(positionLabel, 405);
    lv_label_set_long_mode(positionLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(positionLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(positionLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(positionLabel, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    return row;
}

}  // namespace

void create() {
    resetScreen();
    createHeader(App::Localization::text("Slysene APRS entity", "Received APRS entities"));

    referenceLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(referenceLabel, "Ref: --");
    lv_obj_set_style_text_font(referenceLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(referenceLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(referenceLabel, LV_ALIGN_TOP_RIGHT, -78, 17);

    countLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(countLabel, "0/15");
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
    currentCount = 0;
}

void update(
    const Services::StationStore::ViewState& state,
    const Services::PositionReference& reference) {

    if (listObject == nullptr || countLabel == nullptr || referenceLabel == nullptr) {
        return;
    }
    if (selected >= state.count && state.count > 0) {
        selected = state.count - 1;
    }
    if (renderedRevision == state.revision &&
        renderedReferenceRevision == reference.revision &&
        currentCount == state.count) {
        return;
    }

    renderedRevision = state.revision;
    renderedReferenceRevision = reference.revision;
    currentCount = state.count;
    lv_obj_clean(listObject);
    for (lv_obj_t*& row : renderedRows) {
        row = nullptr;
    }
    lv_label_set_text(referenceLabel, reference.valid ? (reference.fromGps ? "Ref: GPS" : "Ref: DEF") : "Ref: --");

    char countText[16];
    std::snprintf(countText, sizeof(countText), "%u/%u",
        static_cast<unsigned>(state.count),
        static_cast<unsigned>(Services::StationStore::MAX_STATIONS));
    lv_label_set_text(countLabel, countText);

    if (state.count == 0) {
        lv_obj_t* emptyLabel = lv_label_create(listObject);
        lv_label_set_text(
            emptyLabel,
            App::Localization::text(
                "Zatim nebyla prijata zadna platna APRS entita.",
                "No valid APRS entity has been received yet."));
        lv_obj_set_width(emptyLabel, 420);
        lv_label_set_long_mode(emptyLabel, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(emptyLabel, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(emptyLabel, lv_color_hex(0x92A7C7), 0);
        return;
    }

    for (std::size_t index = 0; index < state.count; ++index) {
        renderedRows[index] = createStationRow(state.stations[index], reference, index == selected);
    }
    if (renderedRows[selected] != nullptr) {
        lv_obj_scroll_to_view(renderedRows[selected], LV_ANIM_OFF);
    }
}

void moveSelection(int direction) {
    if (direction == 0 || currentCount == 0) {
        return;
    }
    if (direction < 0) {
        selected = selected == 0 ? currentCount - 1 : selected - 1;
    } else {
        selected = (selected + 1) % currentCount;
    }
    renderedRevision = 0xFFFFFFFFU;
}

std::size_t selectedIndex() {
    return selected;
}

}  // namespace StationsScreen
}  // namespace Ui
