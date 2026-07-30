#include "ui/screens/menu_screen.h"

#include <cstdint>
#include <cstdio>
#include <lvgl.h>

#include "app/menu_model.h"
#include "services/geo_utils.h"
#include "ui/icons/aprs_icon_assets.h"
#include "ui/ui_components.h"
#include "ui/ui_styles.h"

namespace Ui {
namespace MenuScreen {
namespace {

lv_color_t colorFromHex(std::uint32_t value) {
    return lv_color_hex(value);
}

lv_obj_t* rows[App::MenuModel::ITEM_COUNT] = {};
lv_obj_t* statusLabel = nullptr;
lv_obj_t* locatorLabel = nullptr;
lv_obj_t* gpsBox = nullptr;
lv_obj_t* gpsGlyph = nullptr;
lv_obj_t* messageBox = nullptr;
lv_obj_t* messageGlyph = nullptr;
lv_obj_t* messageBadge = nullptr;
lv_obj_t* messageBadgeLabel = nullptr;
lv_obj_t* stationBox = nullptr;
lv_obj_t* stationGlyph = nullptr;
lv_obj_t* stationBadge = nullptr;
lv_obj_t* stationBadgeLabel = nullptr;
lv_obj_t* trackerBox = nullptr;
lv_obj_t* trackerImage = nullptr;
lv_obj_t* trailBox = nullptr;
lv_obj_t* trailGlyph = nullptr;
lv_obj_t* digiBox = nullptr;
lv_obj_t* digiImage = nullptr;
lv_obj_t* igateBox = nullptr;
lv_obj_t* igateImage = nullptr;
lv_obj_t* igateOverlay = nullptr;
std::size_t selection = 0;
std::uint32_t renderedReferenceRevision = 0xFFFFFFFFU;

lv_obj_t* createIndicatorContainer(lv_coord_t x) {
    lv_obj_t* box = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, 30, 30);
    lv_obj_align(box, LV_ALIGN_TOP_LEFT, x, 10);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(box, 8, 0);
    lv_obj_set_style_bg_color(box, colorFromHex(0x17243A), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, colorFromHex(0x46566E), 0);
    return box;
}

lv_obj_t* createIndicatorBox(
    lv_coord_t x,
    const char* glyph,
    lv_obj_t*& glyphLabel) {

    lv_obj_t* box = createIndicatorContainer(x);
    glyphLabel = lv_label_create(box);
    lv_label_set_text(glyphLabel, glyph);
    lv_obj_set_style_text_font(glyphLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(glyphLabel, colorFromHex(0x46566E), 0);
    lv_obj_center(glyphLabel);
    return box;
}

lv_obj_t* createAprsIndicatorBox(
    lv_coord_t x,
    const lv_img_dsc_t* descriptor,
    lv_obj_t*& image,
    const char* overlayText = nullptr,
    lv_obj_t** overlay = nullptr) {

    lv_obj_t* box = createIndicatorContainer(x);
    image = lv_img_create(box);
    lv_img_set_src(image, descriptor);
    lv_obj_set_style_img_recolor(image, colorFromHex(0x46566E), 0);
    lv_obj_set_style_img_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_center(image);

    if (overlayText != nullptr && overlayText[0] != '\0' && overlay != nullptr) {
        *overlay = lv_label_create(box);
        lv_label_set_text(*overlay, overlayText);
        lv_obj_set_style_text_font(*overlay, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(*overlay, colorFromHex(0x46566E), 0);
        lv_obj_set_style_bg_color(*overlay, colorFromHex(0x101A2B), 0);
        lv_obj_set_style_bg_opa(*overlay, LV_OPA_70, 0);
        lv_obj_set_style_radius(*overlay, 4, 0);
        lv_obj_set_style_pad_left(*overlay, 1, 0);
        lv_obj_set_style_pad_right(*overlay, 1, 0);
        lv_obj_center(*overlay);
    }
    return box;
}

void createBadge(
    lv_obj_t* box,
    lv_obj_t*& badge,
    lv_obj_t*& badgeLabel) {

    badge = lv_obj_create(box);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, 15, 15);
    lv_obj_align(badge, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(badge, colorFromHex(0xF05B67), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_border_color(badge, colorFromHex(0x101D31), 0);

    badgeLabel = lv_label_create(badge);
    lv_obj_set_style_text_font(badgeLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(badgeLabel, lv_color_white(), 0);
    lv_obj_center(badgeLabel);
    lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);
}

void setIndicatorColor(lv_obj_t* box, lv_obj_t* glyph, lv_color_t color) {
    if (box != nullptr) {
        lv_obj_set_style_border_color(box, color, 0);
    }
    if (glyph != nullptr) {
        lv_obj_set_style_text_color(glyph, color, 0);
    }
}

void setAprsIndicatorColor(
    lv_obj_t* box,
    lv_obj_t* image,
    lv_obj_t* overlay,
    lv_color_t color) {

    if (box != nullptr) {
        lv_obj_set_style_border_color(box, color, 0);
    }
    if (image != nullptr) {
        lv_obj_set_style_img_recolor(image, color, 0);
        lv_obj_set_style_img_recolor_opa(image, LV_OPA_COVER, 0);
    }
    if (overlay != nullptr) {
        lv_obj_set_style_text_color(overlay, color, 0);
    }
}

void renderBadge(
    lv_obj_t* badge,
    lv_obj_t* label,
    std::uint8_t count,
    lv_color_t color) {

    if (badge == nullptr || label == nullptr) {
        return;
    }
    if (count == 0U) {
        lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    char text[2] = {
        static_cast<char>('0' + (count > 9U ? 9U : count)),
        '\0'
    };
    lv_label_set_text(label, text);
    lv_obj_set_style_bg_color(badge, color, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_HIDDEN);
}

void renderIndicators(const IndicatorState& indicators) {
    const lv_color_t inactive = colorFromHex(0x46566E);
    lv_color_t gpsColor = inactive;
    if (indicators.gpsFix) {
        gpsColor = colorFromHex(0x42D392);
    } else if (indicators.gpsReceiverDetected || indicators.gpsNmeaPacket) {
        gpsColor = colorFromHex(0xFFB547);
    } else if (indicators.gpsSerialTraffic) {
        gpsColor = colorFromHex(0xD68B4B);
    }
    setIndicatorColor(gpsBox, gpsGlyph, gpsColor);

    const bool hasUnreadMessages = indicators.unreadMessages > 0U;
    const lv_color_t messageColor = hasUnreadMessages
        ? colorFromHex(0xFFB547)
        : inactive;
    setIndicatorColor(messageBox, messageGlyph, messageColor);
    renderBadge(
        messageBadge,
        messageBadgeLabel,
        indicators.unreadMessages,
        colorFromHex(0xF05B67));

    const bool hasNewStations = indicators.newStations > 0U;
    const lv_color_t stationColor = hasNewStations
        ? colorFromHex(0x56C7FF)
        : inactive;
    setIndicatorColor(stationBox, stationGlyph, stationColor);
    renderBadge(
        stationBadge,
        stationBadgeLabel,
        indicators.newStations,
        colorFromHex(0x2764D8));

    lv_color_t trackerColor = inactive;
    if (indicators.trackerActive) {
        trackerColor = colorFromHex(0x42D392);
    } else if (indicators.trackerConfigured) {
        trackerColor = colorFromHex(0xFFB547);
    }
    setAprsIndicatorColor(trackerBox, trackerImage, nullptr, trackerColor);

    lv_color_t trailColor = inactive;
    if (indicators.trailError) {
        trailColor = colorFromHex(0xF05B67);
    } else if (indicators.trailRecording) {
        trailColor = colorFromHex(0x42D392);
    } else if (indicators.trailPaused || indicators.trailConfigured) {
        trailColor = colorFromHex(0xFFB547);
    }
    setIndicatorColor(trailBox, trailGlyph, trailColor);

    const lv_color_t digiColor = indicators.digiEnabled
        ? colorFromHex(0xB59AFF)
        : inactive;
    setAprsIndicatorColor(digiBox, digiImage, nullptr, digiColor);

    lv_color_t igateColor = inactive;
    if (indicators.igateEnabled) {
        igateColor = indicators.igateVerified
            ? colorFromHex(0x42D392)
            : colorFromHex(0xFFB547);
    }
    setAprsIndicatorColor(igateBox, igateImage, igateOverlay, igateColor);
}

void renderLocator(const Services::PositionReference& reference) {
    if (locatorLabel == nullptr || renderedReferenceRevision == reference.revision) {
        return;
    }
    renderedReferenceRevision = reference.revision;

    char locator[7] = {};
    if (reference.valid && Services::maidenheadLocator(
            reference.latitude,
            reference.longitude,
            locator,
            sizeof(locator))) {
        char text[20];
        std::snprintf(
            text,
            sizeof(text),
            "%s %s",
            reference.fromGps ? "GPS" : "DEF",
            locator);
        lv_label_set_text(locatorLabel, text);
        lv_obj_set_style_text_color(
            locatorLabel,
            reference.fromGps ? colorFromHex(0x42D392) : colorFromHex(0x56C7FF),
            0);
    } else {
        lv_label_set_text(locatorLabel, "LOC ------");
        lv_obj_set_style_text_color(locatorLabel, colorFromHex(0x92A7C7), 0);
    }
}

void refresh() {
    for (std::size_t index = 0; index < App::MenuModel::count(); ++index) {
        lv_obj_remove_style(rows[index], &Styles::menuRowNormal(), LV_PART_MAIN);
        lv_obj_remove_style(rows[index], &Styles::menuRowSelected(), LV_PART_MAIN);
        lv_obj_add_style(
            rows[index],
            index == selection ? &Styles::menuRowSelected() : &Styles::menuRowNormal(),
            LV_PART_MAIN);
    }

    const App::MenuModel::MenuItem& item = App::MenuModel::item(selection);
    lv_label_set_text_fmt(
        statusLabel,
        "%u/%u  %s",
        static_cast<unsigned>(selection + 1),
        static_cast<unsigned>(App::MenuModel::count()),
        item.description);
    lv_obj_set_style_text_color(statusLabel, colorFromHex(0x92A7C7), 0);
    if (rows[selection] != nullptr) {
        lv_obj_scroll_to_view(rows[selection], LV_ANIM_ON);
    }
}

void rowClicked(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }
    selection = static_cast<std::size_t>(
        reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
    refresh();
}

}  // namespace

void create(
    std::size_t selectedIndex,
    const Services::PositionReference& reference,
    const IndicatorState& indicators) {

    selection = selectedIndex % App::MenuModel::count();
    resetScreen();
    createHeader("LoRa");

    gpsBox = createIndicatorBox(82, LV_SYMBOL_GPS, gpsGlyph);
    messageBox = createIndicatorBox(116, LV_SYMBOL_BELL, messageGlyph);
    createBadge(messageBox, messageBadge, messageBadgeLabel);
    stationBox = createIndicatorBox(150, LV_SYMBOL_WIFI, stationGlyph);
    createBadge(stationBox, stationBadge, stationBadgeLabel);
    trackerBox = createAprsIndicatorBox(184, &AprsIconAssets::car, trackerImage);
    trailBox = createIndicatorBox(218, LV_SYMBOL_SAVE, trailGlyph);
    digiBox = createAprsIndicatorBox(252, &AprsIconAssets::digipeater, digiImage);
    igateBox = createAprsIndicatorBox(286, &AprsIconAssets::gateway, igateImage, "L", &igateOverlay);
    renderIndicators(indicators);

    locatorLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(locatorLabel, 126);
    lv_label_set_long_mode(locatorLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(locatorLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(locatorLabel, &lv_font_montserrat_16, 0);
    lv_obj_align(locatorLabel, LV_ALIGN_BOTTOM_RIGHT, -14, -72);
    renderedReferenceRevision = 0xFFFFFFFFU;
    renderLocator(reference);

    lv_obj_t* content = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, 452, 168);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_pad_row(content, 3, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (std::size_t index = 0; index < App::MenuModel::count(); ++index) {
        const App::MenuModel::MenuItem& item = App::MenuModel::item(index);
        rows[index] = lv_obj_create(content);
        lv_obj_set_size(rows[index], 444, 30);
        lv_obj_clear_flag(rows[index], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(rows[index], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(
            rows[index],
            rowClicked,
            LV_EVENT_CLICKED,
            reinterpret_cast<void*>(static_cast<std::intptr_t>(index)));

        lv_obj_t* label = lv_label_create(rows[index]);
        lv_label_set_text(label, item.title);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    }

    statusLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(statusLabel, 304);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(statusLabel, colorFromHex(0x92A7C7), 0);
    lv_obj_align(statusLabel, LV_ALIGN_BOTTOM_LEFT, 14, -72);

    refresh();
}

void update(
    const Services::PositionReference& reference,
    const IndicatorState& indicators) {

    renderLocator(reference);
    renderIndicators(indicators);
}

void setMessage(const char* text) {
    if (statusLabel != nullptr) {
        lv_label_set_text(statusLabel, text != nullptr ? text : "");
        lv_obj_set_style_text_color(statusLabel, colorFromHex(0x42D392), 0);
    }
}

void moveSelection(int direction) {
    const std::size_t count = App::MenuModel::count();
    if (direction < 0) {
        selection = selection == 0 ? count - 1 : selection - 1;
    } else if (direction > 0) {
        selection = (selection + 1) % count;
    }
    refresh();
}

std::size_t selectedIndex() {
    return selection;
}

}  // namespace MenuScreen
}  // namespace Ui
