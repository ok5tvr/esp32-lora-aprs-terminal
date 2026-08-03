#include "ui/screens/beacon_screen.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "app/aprs_path.h"
#include "app/tracker_symbols.h"
#include "ui/ui_components.h"

namespace Ui {
namespace BeaconScreen {
namespace {

enum class Field : std::uint8_t {
    Source,
    Path,
    Comment
};

lv_obj_t* content = nullptr;
lv_obj_t* gpsLabel = nullptr;
lv_obj_t* detailLabel = nullptr;
lv_obj_t* messageLabel = nullptr;
lv_obj_t* valueLabels[3] = {};
lv_obj_t* editorOverlay = nullptr;
lv_obj_t* editorArea = nullptr;

App::BeaconActionHandler handler = nullptr;
void* handlerContext = nullptr;
App::TrackerPositionSource draftSource = App::TrackerPositionSource::DefaultPosition;
App::AprsPath draftPath = App::AprsPath::Wide1_1;
char draftComment[Services::SettingsService::APRS_COMMENT_CAPACITY] = {};
App::UiLanguage language = App::UiLanguage::Czech;
std::uint32_t observedSettingsRevision = 0xFFFFFFFFU;
bool pendingOpenEditor = false;
bool pendingCloseEditor = false;
bool acceptEditor = false;

bool english() {
    return language == App::UiLanguage::English;
}

const char* text(const char* czech, const char* englishText) {
    return english() ? englishText : czech;
}

void copyText(char* output, std::size_t capacity, const char* value) {
    if (output == nullptr || capacity == 0) {
        return;
    }
    std::snprintf(output, capacity, "%s", value != nullptr ? value : "");
}

const char* sourceText(App::TrackerPositionSource source) {
    return source == App::TrackerPositionSource::Gps
        ? "GPS"
        : text("VYCHOZI", "DEFAULT");
}

bool isSuccessMessage(const char* value) {
    return value != nullptr &&
        (std::strstr(value, "ulozen") != nullptr ||
         std::strstr(value, "zarazen") != nullptr ||
         std::strstr(value, "saved") != nullptr ||
         std::strstr(value, "queued") != nullptr);
}

void refreshDraftLabels() {
    if (valueLabels[0] == nullptr) {
        return;
    }
    lv_label_set_text(valueLabels[0], sourceText(draftSource));
    lv_label_set_text(valueLabels[1], App::aprsPathLabel(draftPath));
    lv_label_set_text(valueLabels[2], draftComment);
}

void copyFromSettings(const Services::SettingsService::ViewState& settings) {
    language = settings.uiLanguage;
    draftSource = settings.beaconSource;
    draftPath = settings.beaconPath;
    copyText(draftComment, sizeof(draftComment), settings.beaconComment);
    observedSettingsRevision = settings.revision;
    refreshDraftLabels();
}

void keyboardEvent(lv_event_t* event) {
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_READY) {
        acceptEditor = true;
        pendingCloseEditor = true;
    } else if (code == LV_EVENT_CANCEL) {
        acceptEditor = false;
        pendingCloseEditor = true;
    }
}

void openCommentEditor() {
    if (editorOverlay != nullptr) {
        return;
    }

    editorOverlay = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(editorOverlay);
    lv_obj_set_size(editorOverlay, 480, 320);
    lv_obj_align(editorOverlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(editorOverlay, lv_color_hex(0x0B1424), 0);
    lv_obj_set_style_bg_opa(editorOverlay, LV_OPA_COVER, 0);
    lv_obj_add_flag(editorOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(editorOverlay);

    lv_obj_t* title = lv_label_create(editorOverlay);
    lv_label_set_text(title, text("Komentar beaconu", "Beacon comment"));
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 14, 8);

    editorArea = lv_textarea_create(editorOverlay);
    lv_obj_set_size(editorArea, 452, 43);
    lv_obj_align(editorArea, LV_ALIGN_TOP_MID, 0, 38);
    lv_textarea_set_one_line(editorArea, true);
    lv_textarea_set_text(editorArea, draftComment);
    lv_textarea_set_max_length(
        editorArea,
        Services::SettingsService::APRS_COMMENT_CAPACITY - 1);
    lv_obj_set_style_text_font(editorArea, &lv_font_montserrat_18, 0);

    lv_obj_t* keyboard = lv_keyboard_create(editorOverlay);
    lv_obj_set_size(keyboard, 468, 228);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_keyboard_set_textarea(keyboard, editorArea);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_event_cb(keyboard, keyboardEvent, LV_EVENT_ALL, nullptr);
    lv_obj_add_state(editorArea, LV_STATE_FOCUSED);
}

void closeCommentEditor() {
    if (editorOverlay == nullptr) {
        return;
    }
    if (acceptEditor && editorArea != nullptr) {
        copyText(
            draftComment,
            sizeof(draftComment),
            lv_textarea_get_text(editorArea));
    }
    lv_obj_del(editorOverlay);
    editorOverlay = nullptr;
    editorArea = nullptr;
    pendingCloseEditor = false;
    acceptEditor = false;
    refreshDraftLabels();
}

void fieldClicked(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }
    const Field field = static_cast<Field>(
        reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
    if (field == Field::Source) {
        draftSource = draftSource == App::TrackerPositionSource::Gps
            ? App::TrackerPositionSource::DefaultPosition
            : App::TrackerPositionSource::Gps;
    } else if (field == Field::Path) {
        draftPath = static_cast<App::AprsPath>(
            (static_cast<std::uint8_t>(draftPath) + 1U) % 3U);
    } else if (field == Field::Comment) {
        pendingOpenEditor = true;
    }
    refreshDraftLabels();
}

void createFieldRow(
    const char* title,
    const char* hint,
    Field field,
    std::size_t valueIndex) {

    lv_obj_t* row = lv_obj_create(content);
    lv_obj_set_size(row, 438, 46);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 9, 0);
    lv_obj_set_style_pad_left(row, 10, 0);
    lv_obj_set_style_pad_right(row, 10, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(
        row,
        fieldClicked,
        LV_EVENT_CLICKED,
        reinterpret_cast<void*>(static_cast<std::intptr_t>(field)));

    lv_obj_t* titleLabel = lv_label_create(row);
    lv_label_set_text(titleLabel, title);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 0, 2);

    lv_obj_t* hintLabel = lv_label_create(row);
    lv_label_set_text(hintLabel, hint);
    lv_obj_set_style_text_font(hintLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hintLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(hintLabel, LV_ALIGN_BOTTOM_LEFT, 0, -1);

    valueLabels[valueIndex] = lv_label_create(row);
    lv_obj_set_width(valueLabels[valueIndex], valueIndex == 2 ? 230 : 120);
    lv_label_set_long_mode(valueLabels[valueIndex], LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(valueLabels[valueIndex], &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(valueLabels[valueIndex], lv_color_hex(0x56C7FF), 0);
    lv_obj_align(valueLabels[valueIndex], LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_align(valueLabels[valueIndex], LV_TEXT_ALIGN_RIGHT, 0);
}

void perform(bool transmit) {
    if (handler == nullptr) {
        return;
    }
    char error[160] = {};
    const bool result = handler(
        draftSource,
        draftPath,
        draftComment,
        transmit,
        error,
        sizeof(error),
        handlerContext);
    if (result) {
        setMessage(transmit
            ? text(
                "Beacon byl zarazen do TX fronty.",
                "The beacon was queued for transmission.")
            : text(
                "Nastaveni beaconu bylo ulozeno do NVS.",
                "Beacon settings were saved to NVS."));
    } else {
        setMessage(error);
    }
}

void saveClicked(lv_event_t* event) {
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        save();
    }
}

void sendClicked(lv_event_t* event) {
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        send();
    }
}

void createActionButton(const char* caption, lv_event_cb_t callback, std::uint32_t color) {
    lv_obj_t* button = lv_btn_create(content);
    lv_obj_set_size(button, 438, 40);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text(label, caption);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);
}

}  // namespace

void create(
    const Services::SettingsService::ViewState& settings,
    const Services::GpsService::ViewState& gps,
    App::BeaconActionHandler actionHandler,
    void* actionContext) {

    handler = actionHandler;
    handlerContext = actionContext;
    language = settings.uiLanguage;
    editorOverlay = nullptr;
    editorArea = nullptr;
    pendingOpenEditor = false;
    pendingCloseEditor = false;
    acceptEditor = false;
    for (lv_obj_t*& label : valueLabels) {
        label = nullptr;
    }

    resetScreen();
    createHeader(text("APRS beacon", "APRS beacon"));

    gpsLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(gpsLabel, 225);
    lv_label_set_long_mode(gpsLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(gpsLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(gpsLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(gpsLabel, LV_ALIGN_TOP_LEFT, 14, 52);

    detailLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(detailLabel, 225);
    lv_label_set_long_mode(detailLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(detailLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(detailLabel, lv_color_hex(0x42D392), 0);
    lv_obj_align(detailLabel, LV_ALIGN_TOP_RIGHT, -14, 52);

    content = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, 452, 164);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 76);
    lv_obj_set_style_pad_left(content, 5, 0);
    lv_obj_set_style_pad_right(content, 5, 0);
    lv_obj_set_style_pad_top(content, 2, 0);
    lv_obj_set_style_pad_bottom(content, 2, 0);
    lv_obj_set_style_pad_row(content, 5, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);

    createFieldRow(
        text("Zdroj pozice", "Position source"),
        text("Aktualni GPS nebo vychozi QTH", "Current GPS or default QTH"),
        Field::Source,
        0);
    createFieldRow(
        "APRS path",
        text("Direct / WIDE1-1 / WIDE2-2", "Direct / WIDE1-1 / WIDE2-2"),
        Field::Path,
        1);
    createFieldRow(
        text("Komentar", "Comment"),
        text("Maximalne 48 znaku", "Maximum 48 characters"),
        Field::Comment,
        2);

    createActionButton(
        text("Ulozit nastaveni beaconu", "Save beacon settings"),
        saveClicked,
        0x2764D8);
    createActionButton(
        text("Odeslat beacon nyni", "Send beacon now"),
        sendClicked,
        0x168A61);

    messageLabel = lv_label_create(content);
    lv_obj_set_width(messageLabel, 430);
    lv_label_set_long_mode(messageLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(messageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(messageLabel, lv_color_hex(0x92A7C7), 0);
    lv_label_set_text(
        messageLabel,
        text(
            "Format a APRS symbol se prebiraji z nastaveni Trackeru.",
            "The format and APRS symbol are taken from Tracker settings."));

    copyFromSettings(settings);
    update(gps, settings);
}

void update(
    const Services::GpsService::ViewState& gps,
    const Services::SettingsService::ViewState& settings) {

    if (gpsLabel == nullptr || detailLabel == nullptr) {
        return;
    }
    if (settings.revision != observedSettingsRevision) {
        copyFromSettings(settings);
    }

    char gpsText[96] = {};
    if (!gps.receiverDetected) {
        std::snprintf(gpsText, sizeof(gpsText), "%s", text("GPS: nenalezena", "GPS: not detected"));
    } else if (!gps.hasFix) {
        std::snprintf(
            gpsText,
            sizeof(gpsText),
            english() ? "GPS: no fix, sat %u" : "GPS: bez fixu, sat %u",
            static_cast<unsigned>(gps.satellites));
    } else {
        std::snprintf(
            gpsText,
            sizeof(gpsText),
            "GPS: %.5f, %.5f",
            gps.latitude,
            gps.longitude);
    }
    lv_label_set_text(gpsLabel, gpsText);

    const App::TrackerSymbolDefinition& symbol =
        App::trackerSymbolDefinition(settings.trackerSymbol);
    char details[96] = {};
    std::snprintf(
        details,
        sizeof(details),
        english() ? "Format %s | symbol %c%c" : "Format %s | symbol %c%c",
        settings.trackerFormat == App::TrackerPositionFormat::Compressed
            ? text("compressed", "compressed")
            : text("normal", "standard"),
        symbol.table,
        symbol.code);
    lv_label_set_text(detailLabel, details);
}

void save() {
    perform(false);
}

void send() {
    perform(true);
}

void processPending() {
    if (pendingOpenEditor) {
        pendingOpenEditor = false;
        openCommentEditor();
    }
    if (pendingCloseEditor) {
        closeCommentEditor();
    }
}

void scroll(int direction) {
    if (content == nullptr || direction == 0) {
        return;
    }
    lv_obj_scroll_by(content, 0, direction > 0 ? -47 : 47, LV_ANIM_ON);
}

void setMessage(const char* value) {
    if (messageLabel == nullptr) {
        return;
    }
    lv_label_set_text(messageLabel, value != nullptr ? value : "");
    lv_obj_set_style_text_color(
        messageLabel,
        isSuccessMessage(value)
            ? lv_color_hex(0x42D392)
            : lv_color_hex(0xFFB454),
        0);
}

}  // namespace BeaconScreen
}  // namespace Ui
