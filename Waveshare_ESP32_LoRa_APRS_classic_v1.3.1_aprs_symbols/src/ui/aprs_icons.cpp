#include "ui/aprs_icons.h"

#include "ui/aprs_symbol_lookup.h"
#include "ui/icons/aprs_icon_assets.h"

namespace Ui::AprsIcons {

static_assert(
    AprsIconAssets::SYMBOL_COUNT == AprsSymbols::SYMBOL_COUNT,
    "APRS image assets and symbol lookup must contain the same number of entries");

namespace {

void styleOverlayLabel(lv_obj_t* label) {
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_bg_color(label, lv_color_hex(0x101A2B), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_80, 0);
    lv_obj_set_style_radius(label, 5, 0);
    lv_obj_set_style_pad_left(label, 2, 0);
    lv_obj_set_style_pad_right(label, 2, 0);
    lv_obj_set_style_pad_top(label, 0, 0);
    lv_obj_set_style_pad_bottom(label, 0, 0);
}

void styleFallbackLabel(lv_obj_t* label) {
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x263244), 0);
    lv_obj_set_style_bg_color(label, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_80, 0);
    lv_obj_set_style_radius(label, 4, 0);
    lv_obj_set_style_pad_left(label, 1, 0);
    lv_obj_set_style_pad_right(label, 1, 0);
}

}  // namespace

lv_obj_t* create(
    lv_obj_t* parent,
    char symbolTable,
    char symbolCode,
    bool symbolAvailable) {

    const AprsSymbols::Lookup lookup =
        AprsSymbols::resolve(symbolTable, symbolCode, symbolAvailable);

    const lv_img_dsc_t* descriptor = &AprsIconAssets::unknown;
    if (lookup.valid) {
        descriptor = lookup.alternate
            ? &AprsIconAssets::alternate[lookup.index]
            : &AprsIconAssets::primary[lookup.index];
    }

    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, 32, 32);
    lv_obj_set_style_bg_color(container, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(container, 3, 0);
    lv_obj_set_style_clip_corner(container, true, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* image = lv_img_create(container);
    lv_img_set_src(image, descriptor);
    lv_obj_center(image);

    if (lookup.overlay != '\0') {
        char overlayText[2] = {lookup.overlay, '\0'};
        lv_obj_t* overlayLabel = lv_label_create(container);
        lv_label_set_text(overlayLabel, overlayText);
        styleOverlayLabel(overlayLabel);
        lv_obj_center(overlayLabel);
    } else if (!lookup.valid && symbolAvailable) {
        char fallbackText[3] = {symbolTable, symbolCode, '\0'};
        lv_obj_t* fallbackLabel = lv_label_create(container);
        lv_label_set_text(fallbackLabel, fallbackText);
        styleFallbackLabel(fallbackLabel);
        lv_obj_center(fallbackLabel);
    }

    return container;
}

}  // namespace Ui::AprsIcons
