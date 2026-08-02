#include "ui/screens/station_detail_screen.h"

#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "app/localization.h"
#include "services/geo_utils.h"
#include "ui/aprs_icons.h"
#include "ui/ui_components.h"

namespace Ui {
namespace StationDetailScreen {
namespace {

lv_obj_t* contentObject = nullptr;
lv_obj_t* titleLabel = nullptr;
lv_obj_t* identityLabel = nullptr;
lv_obj_t* positionLabel = nullptr;
lv_obj_t* signalLabel = nullptr;
lv_obj_t* routeLabel = nullptr;
lv_obj_t* packetLabel = nullptr;
lv_obj_t* actionLabel = nullptr;
lv_obj_t* iconObject = nullptr;
char renderedSymbolTable = '\0';
char renderedSymbolCode = '\0';
bool renderedHasPosition = false;

const char* entityTypeName(Aprs::EntityType type) {
    switch (type) {
        case Aprs::EntityType::Object: return App::Localization::text("Objekt", "Object");
        case Aprs::EntityType::Item: return App::Localization::text("Polozka", "Item");
        default: return App::Localization::text("Stanice", "Station");
    }
}

const char* positionFormatName(Aprs::PositionFormat format) {
    switch (format) {
        case Aprs::PositionFormat::Uncompressed: return App::Localization::text("normalni", "standard");
        case Aprs::PositionFormat::Compressed: return App::Localization::text("komprimovana", "compressed");
        case Aprs::PositionFormat::MicE: return "Mic-E";
        default: return App::Localization::text("bez polohy", "no position");
    }
}

void formatAge(char* output, std::size_t capacity, std::uint32_t ageMs) {
    const std::uint32_t seconds = ageMs / 1000U;
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
    iconObject = nullptr;
    renderedSymbolTable = '\0';
    renderedSymbolCode = '\0';
    renderedHasPosition = false;
    createHeader(App::Localization::text("Detail APRS entity", "APRS entity details"));

    contentObject = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(contentObject);
    lv_obj_set_size(contentObject, 452, 194);
    lv_obj_align(contentObject, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_pad_left(contentObject, 5, 0);
    lv_obj_set_style_pad_right(contentObject, 5, 0);
    lv_obj_set_style_pad_top(contentObject, 2, 0);
    lv_obj_set_style_pad_bottom(contentObject, 2, 0);
    lv_obj_set_scroll_dir(contentObject, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(contentObject, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t* card = lv_obj_create(contentObject);
    lv_obj_set_size(card, 438, 430);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    titleLabel = lv_label_create(card);
    lv_obj_set_width(titleLabel, 350);
    lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    identityLabel = lv_label_create(card);
    lv_obj_set_width(identityLabel, 390);
    lv_label_set_long_mode(identityLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(identityLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(identityLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(identityLabel, LV_ALIGN_TOP_LEFT, 0, 30);

    positionLabel = lv_label_create(card);
    lv_obj_set_width(positionLabel, 414);
    lv_obj_set_height(positionLabel, 48);
    lv_label_set_long_mode(positionLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(positionLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(positionLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(positionLabel, LV_ALIGN_TOP_LEFT, 0, 55);

    signalLabel = lv_label_create(card);
    lv_obj_set_width(signalLabel, 414);
    lv_obj_set_height(signalLabel, 38);
    lv_label_set_long_mode(signalLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(signalLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(signalLabel, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(signalLabel, LV_ALIGN_TOP_LEFT, 0, 105);

    routeLabel = lv_label_create(card);
    lv_obj_set_width(routeLabel, 414);
    lv_obj_set_height(routeLabel, 126);
    lv_label_set_long_mode(routeLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(routeLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(routeLabel, lv_color_hex(0x42D392), 0);
    lv_obj_align(routeLabel, LV_ALIGN_TOP_LEFT, 0, 148);

    packetLabel = lv_label_create(card);
    lv_obj_set_width(packetLabel, 414);
    lv_obj_set_height(packetLabel, 124);
    lv_label_set_long_mode(packetLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(packetLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(packetLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(packetLabel, LV_ALIGN_TOP_LEFT, 0, 284);

    actionLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(
        actionLabel,
        App::Localization::text(
            "Nahoru/Dolu = detail | OK = navigace",
            "Up/Down = details | OK = navigation"));
    lv_obj_set_style_text_font(actionLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(actionLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(actionLabel, LV_ALIGN_BOTTOM_MID, 0, -72);
}

void scroll(int direction) {
    if (contentObject == nullptr || direction == 0) {
        return;
    }
    lv_obj_scroll_by(contentObject, 0, direction > 0 ? -58 : 58, LV_ANIM_ON);
}

void update(
    const Services::StationStore::Station& station,
    const Services::PositionReference& reference,
    std::uint32_t now) {

    if (titleLabel == nullptr) {
        return;
    }

    lv_label_set_text(
        titleLabel,
        station.type == Aprs::EntityType::Station ? station.callsign : station.entityName);
    lv_obj_set_style_text_color(
        titleLabel,
        lv_color_hex(station.emergency ? 0xFF4242 : 0xF4F7FF),
        0);

    if (iconObject == nullptr || renderedSymbolTable != station.symbol[0] ||
        renderedSymbolCode != station.symbol[1] || renderedHasPosition != station.hasPosition) {
        if (iconObject != nullptr) {
            lv_obj_del(iconObject);
        }
        iconObject = AprsIcons::create(
            lv_obj_get_parent(titleLabel),
            station.symbol[0],
            station.symbol[1],
            station.hasPosition);
        lv_obj_align(iconObject, LV_ALIGN_TOP_RIGHT, -2, 5);
        renderedSymbolTable = station.symbol[0];
        renderedSymbolCode = station.symbol[1];
        renderedHasPosition = station.hasPosition;
    }

    char age[24];
    formatAge(age, sizeof(age), now - station.lastHeardMs);
    lv_label_set_text_fmt(
        identityLabel,
        App::Localization::text(
            "%s | zdroj %s | naposledy %s | paketu %lu",
            "%s | source %s | last heard %s | packets %lu"),
        entityTypeName(station.type),
        station.callsign,
        age,
        static_cast<unsigned long>(station.heardCount));

    if (station.hasPosition) {
        const Services::DistanceBearing relative = reference.valid
            ? Services::calculateDistanceBearing(
                reference.latitude,
                reference.longitude,
                station.latitude,
                station.longitude)
            : Services::DistanceBearing{};
        if (relative.valid) {
            lv_label_set_text_fmt(
                positionLabel,
                App::Localization::text(
                    "Poloha: %.5f, %.5f\nVzdalenost: %.2f km | azimut %03.0f deg %s | ref. %s",
                    "Position: %.5f, %.5f\nDistance: %.2f km | bearing %03.0f deg %s | ref. %s"),
                station.latitude,
                station.longitude,
                relative.distanceKm,
                relative.bearingDegrees,
                Services::cardinalDirection(relative.bearingDegrees),
                reference.fromGps ? "GPS" : "DEF");
        } else {
            lv_label_set_text_fmt(
                positionLabel,
                App::Localization::text(
                    "Poloha: %.5f, %.5f\nVzdalenost: --",
                    "Position: %.5f, %.5f\nDistance: --"),
                station.latitude,
                station.longitude);
        }
    } else {
        lv_label_set_text(
            positionLabel,
            App::Localization::text(
                "Poloha nebyla v poslednim paketu uvedena.\nNavigace neni dostupna.",
                "The latest packet contains no position.\nNavigation is unavailable."));
    }

    char protocol[220] = {};
    std::snprintf(
        protocol,
        sizeof(protocol),
        App::Localization::text(
            "Format %s | symbol %c%c | RSSI %.1f | SNR %.1f",
            "Format %s | symbol %c%c | RSSI %.1f | SNR %.1f"),
        positionFormatName(station.positionFormat),
        station.symbol[0],
        station.symbol[1],
        static_cast<double>(station.lastRssiDbm),
        static_cast<double>(station.lastSnrDb));
    if (station.emergency) {
        std::strncat(protocol, " | EMERGENCY", sizeof(protocol) - std::strlen(protocol) - 1);
    }
    if (station.frequency.valid) {
        char part[64];
        std::snprintf(
            part,
            sizeof(part),
            " | %.3f MHz",
            static_cast<double>(station.frequency.frequencyMhz));
        std::strncat(protocol, part, sizeof(protocol) - std::strlen(protocol) - 1);
    }
    if (station.phg.valid) {
        char part[72];
        std::snprintf(
            part,
            sizeof(part),
            " | PHG %uW/%luft/%udB/%u",
            station.phg.powerWatts,
            static_cast<unsigned long>(station.phg.heightFeet),
            station.phg.gainDb,
            station.phg.directivityDegrees);
        std::strncat(protocol, part, sizeof(protocol) - std::strlen(protocol) - 1);
    }
    if (station.telemetry.valid) {
        char part[48];
        std::snprintf(part, sizeof(part), " | T#%03u", station.telemetry.sequence);
        std::strncat(protocol, part, sizeof(protocol) - std::strlen(protocol) - 1);
    }
    lv_label_set_text(signalLabel, protocol);

    char directAge[40] = {};
    if (station.hasDirectReception) {
        char elapsed[24] = {};
        formatAge(elapsed, sizeof(elapsed), now - station.lastDirectHeardMs);
        std::snprintf(
            directAge,
            sizeof(directAge),
            App::Localization::text("pred %s", "%s ago"),
            elapsed);
    } else {
        std::snprintf(
            directAge,
            sizeof(directAge),
            "%s",
            App::Localization::text("nikdy", "never"));
    }

    const char* pathText = station.path[0] != '\0'
        ? station.path
        : App::Localization::text("bez cesty", "no path");
    char routeText[480] = {};
    if (station.lastReceptionDirect) {
        std::snprintf(
            routeText,
            sizeof(routeText),
            App::Localization::text(
                "DIRECT | DIGI skoku 0\nPrime %lu | opakovane %lu | posledni primy %s\nCesta: %s",
                "DIRECT | DIGI hops 0\nDirect %lu | repeated %lu | last direct %s\nPath: %s"),
            static_cast<unsigned long>(station.directReceptionCount),
            static_cast<unsigned long>(station.repeatedReceptionCount),
            directAge,
            pathText);
        lv_obj_set_style_text_color(routeLabel, lv_color_hex(0x42D392), 0);
    } else {
        const char* lastDigi = station.lastDigipeater[0] != '\0'
            ? station.lastDigipeater
            : "--";
        std::snprintf(
            routeText,
            sizeof(routeText),
            App::Localization::text(
                "PRES DIGI | skoku %u | posledni %s\nPrime %lu | opakovane %lu | posledni primy %s\nCesta: %s",
                "VIA DIGI | hops %u | last %s\nDirect %lu | repeated %lu | last direct %s\nPath: %s"),
            static_cast<unsigned>(station.digipeaterHops),
            lastDigi,
            static_cast<unsigned long>(station.directReceptionCount),
            static_cast<unsigned long>(station.repeatedReceptionCount),
            directAge,
            pathText);
        lv_obj_set_style_text_color(routeLabel, lv_color_hex(0xFFB454), 0);
    }
    lv_label_set_text(routeLabel, routeText);

    char packetText[232] = {};
    std::snprintf(
        packetText,
        sizeof(packetText),
        "TNC2:\n%s",
        station.lastFrame[0] != '\0'
            ? station.lastFrame
            : App::Localization::text(
                "Posledni TNC2 ramec neni ulozen.",
                "The latest TNC2 frame is not stored."));
    lv_label_set_text(packetLabel, packetText);

    lv_label_set_text(
        actionLabel,
        station.hasPosition
            ? App::Localization::text(
                "Nahoru/Dolu = detail | OK = navigace",
                "Up/Down = details | OK = navigation")
            : App::Localization::text(
                "Nahoru/Dolu = detail | bez polohy",
                "Up/Down = details | no position"));
}

}  // namespace StationDetailScreen
}  // namespace Ui
