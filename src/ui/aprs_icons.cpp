#include "ui/aprs_icons.h"

#include <cctype>
#include <cstdint>

#include "ui/icons/aprs_icon_assets.h"

namespace Ui::AprsIcons {
namespace {

struct ResolvedIcon {
    const lv_img_dsc_t* image = &AprsIconAssets::unknown;
    std::uint32_t color = 0x92A7C7;
    char overlay = '\0';
    bool known = false;
};

bool isOverlayTable(char symbolTable) {
    return (symbolTable >= '0' && symbolTable <= '9') ||
           (symbolTable >= 'A' && symbolTable <= 'Z') ||
           (symbolTable >= 'a' && symbolTable <= 'z');
}

char normalizeOverlay(char symbolTable) {
    // APRS compressed positions use a..j for numeric overlays 0..9.
    if (symbolTable >= 'a' && symbolTable <= 'j') {
        return static_cast<char>('0' + (symbolTable - 'a'));
    }
    return symbolTable;
}

ResolvedIcon resolve(char symbolTable, char symbolCode, bool symbolAvailable) {
    ResolvedIcon result;
    if (!symbolAvailable || symbolCode < '!' || symbolCode > '~') {
        return result;
    }

    if (isOverlayTable(symbolTable)) {
        result.overlay = normalizeOverlay(symbolTable);
    }

    switch (symbolCode) {
        case '>':  // car
        case 'j':  // jeep
        case 'k':  // truck
        case 'u':  // bus / vehicle family
        case 'v':  // van
            result.image = &AprsIconAssets::car;
            result.color = 0x56C7FF;
            result.known = true;
            break;

        case '[':  // person
            result.image = &AprsIconAssets::person;
            result.color = 0x56C7FF;
            result.known = true;
            break;

        case '-':  // house / fixed QTH
            result.image = &AprsIconAssets::house;
            result.color = 0x69D6B5;
            result.known = true;
            break;

        case '_':  // weather station
        case 'W':  // weather-related symbol
            result.image = &AprsIconAssets::weather;
            result.color = 0xF7C95C;
            result.known = true;
            break;

        case '#':  // digipeater
            result.image = &AprsIconAssets::digipeater;
            result.color = 0xB59AFF;
            result.known = true;
            break;

        case '&':  // gateway; L& is the LoRa iGate overlay
            result.image = &AprsIconAssets::gateway;
            result.color = 0x69D6B5;
            result.known = true;
            break;

        case 'O':  // balloon
            result.image = &AprsIconAssets::balloon;
            result.color = 0xF28BC8;
            result.known = true;
            break;

        case '^':  // large aircraft
        case '\'': // small aircraft
            result.image = &AprsIconAssets::aircraft;
            result.color = 0x56C7FF;
            result.known = true;
            break;

        case 's':  // boat / ship family
            result.image = &AprsIconAssets::boat;
            result.color = 0x56C7FF;
            result.known = true;
            break;

        case 'b':  // bicycle
        case '<':  // motorcycle
            result.image = &AprsIconAssets::bicycle;
            result.color = 0x69D6B5;
            result.known = true;
            break;

        case 'r':  // repeater
        case 'Y':  // radio / device family
            result.image = &AprsIconAssets::repeater;
            result.color = 0xB59AFF;
            result.known = true;
            break;

        case '.':  // generic point
        case '/':  // default dot
            result.image = &AprsIconAssets::station;
            result.color = 0x92A7C7;
            result.known = true;
            break;

        default:
            break;
    }

    return result;
}

void styleOverlayLabel(lv_obj_t* label) {
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_bg_color(label, lv_color_hex(0x101A2B), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_70, 0);
    lv_obj_set_style_radius(label, 5, 0);
    lv_obj_set_style_pad_left(label, 2, 0);
    lv_obj_set_style_pad_right(label, 2, 0);
    lv_obj_set_style_pad_top(label, 0, 0);
    lv_obj_set_style_pad_bottom(label, 0, 0);
}

}  // namespace

lv_obj_t* create(
    lv_obj_t* parent,
    char symbolTable,
    char symbolCode,
    bool symbolAvailable) {

    const ResolvedIcon resolved = resolve(symbolTable, symbolCode, symbolAvailable);

    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, 32, 32);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* image = lv_img_create(container);
    lv_img_set_src(image, resolved.image);
    lv_obj_set_style_img_recolor(image, lv_color_hex(resolved.color), 0);
    lv_obj_set_style_img_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_center(image);

    if (resolved.overlay != '\0') {
        char overlayText[2] = {resolved.overlay, '\0'};
        lv_obj_t* overlayLabel = lv_label_create(container);
        lv_label_set_text(overlayLabel, overlayText);
        styleOverlayLabel(overlayLabel);
        lv_obj_center(overlayLabel);
    } else if (!resolved.known && symbolAvailable) {
        char fallbackText[3] = {symbolTable, symbolCode, '\0'};
        lv_obj_t* fallbackLabel = lv_label_create(container);
        lv_label_set_text(fallbackLabel, fallbackText);
        lv_obj_set_style_text_font(fallbackLabel, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(fallbackLabel, lv_color_hex(0xF4F7FF), 0);
        lv_obj_set_style_bg_color(fallbackLabel, lv_color_hex(0x101A2B), 0);
        lv_obj_set_style_bg_opa(fallbackLabel, LV_OPA_60, 0);
        lv_obj_set_style_radius(fallbackLabel, 4, 0);
        lv_obj_set_style_pad_left(fallbackLabel, 1, 0);
        lv_obj_set_style_pad_right(fallbackLabel, 1, 0);
        lv_obj_center(fallbackLabel);
    }

    return container;
}

}  // namespace Ui::AprsIcons
