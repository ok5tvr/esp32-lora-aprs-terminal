#include "ui/screens/settings_screen.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <lvgl.h>

#include "ui/ui_components.h"

namespace Ui {
namespace SettingsScreen {
namespace {

enum class Field : std::uint8_t {
    None,
    Callsign,
    Latitude,
    Longitude
};

App::SettingsSaveHandler saveHandler = nullptr;
void* saveContext = nullptr;

char callsignDraft[Services::SettingsService::CALLSIGN_CAPACITY] = {};
char latitudeDraft[24] = {};
char longitudeDraft[24] = {};

lv_obj_t* callsignArea = nullptr;
lv_obj_t* latitudeArea = nullptr;
lv_obj_t* longitudeArea = nullptr;
lv_obj_t* messageLabel = nullptr;
lv_obj_t* editorOverlay = nullptr;
lv_obj_t* editorArea = nullptr;

Field pendingOpen = Field::None;
bool pendingClose = false;
bool acceptEditor = false;
Field activeField = Field::None;

void copyText(char* destination, std::size_t capacity, const char* source) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    std::snprintf(destination, capacity, "%s", source != nullptr ? source : "");
}

void fieldClicked(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || editorOverlay != nullptr) {
        return;
    }
    pendingOpen = static_cast<Field>(
        reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
}

void saveClicked(lv_event_t* event) {
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        save();
    }
}

void keyboardEvent(lv_event_t* event) {
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_READY) {
        acceptEditor = true;
        pendingClose = true;
    } else if (code == LV_EVENT_CANCEL) {
        acceptEditor = false;
        pendingClose = true;
    }
}

void createFieldRow(
    lv_obj_t* parent,
    const char* labelText,
    const char* value,
    Field field,
    lv_coord_t y,
    lv_obj_t*& area) {

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, labelText);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 4, y + 9);

    area = lv_textarea_create(parent);
    lv_obj_set_size(area, 300, 39);
    lv_obj_align(area, LV_ALIGN_TOP_RIGHT, -2, y);
    lv_textarea_set_one_line(area, true);
    lv_textarea_set_text(area, value);
    lv_obj_set_style_text_font(area, &lv_font_montserrat_16, 0);
    lv_obj_add_event_cb(
        area,
        fieldClicked,
        LV_EVENT_CLICKED,
        reinterpret_cast<void*>(static_cast<std::intptr_t>(field)));
}

void openEditor(Field field) {
    if (field == Field::None || editorOverlay != nullptr) {
        return;
    }
    activeField = field;

    editorOverlay = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(editorOverlay);
    lv_obj_set_size(editorOverlay, 480, 320);
    lv_obj_align(editorOverlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(editorOverlay, lv_color_hex(0x0B1424), 0);
    lv_obj_set_style_bg_opa(editorOverlay, LV_OPA_COVER, 0);
    lv_obj_add_flag(editorOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(editorOverlay);

    lv_obj_t* title = lv_label_create(editorOverlay);
    const char* titleText = field == Field::Callsign
        ? "Editace CALL"
        : (field == Field::Latitude ? "Vychozi zemepisna sirka" : "Vychozi zemepisna delka");
    lv_label_set_text(title, titleText);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 14, 8);

    editorArea = lv_textarea_create(editorOverlay);
    lv_obj_set_size(editorArea, 452, 43);
    lv_obj_align(editorArea, LV_ALIGN_TOP_MID, 0, 38);
    lv_textarea_set_one_line(editorArea, true);
    lv_obj_set_style_text_font(editorArea, &lv_font_montserrat_18, 0);

    if (field == Field::Callsign) {
        lv_textarea_set_text(editorArea, callsignDraft);
        lv_textarea_set_max_length(editorArea, Services::SettingsService::CALLSIGN_CAPACITY - 1);
        lv_textarea_set_accepted_chars(editorArea, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-");
    } else if (field == Field::Latitude) {
        lv_textarea_set_text(editorArea, latitudeDraft);
        lv_textarea_set_max_length(editorArea, sizeof(latitudeDraft) - 1);
        lv_textarea_set_accepted_chars(editorArea, "0123456789.-");
    } else {
        lv_textarea_set_text(editorArea, longitudeDraft);
        lv_textarea_set_max_length(editorArea, sizeof(longitudeDraft) - 1);
        lv_textarea_set_accepted_chars(editorArea, "0123456789.-");
    }

    lv_obj_t* keyboard = lv_keyboard_create(editorOverlay);
    lv_obj_set_size(keyboard, 468, 228);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_keyboard_set_textarea(keyboard, editorArea);
    lv_keyboard_set_mode(
        keyboard,
        field == Field::Callsign ? LV_KEYBOARD_MODE_TEXT_UPPER : LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_event_cb(keyboard, keyboardEvent, LV_EVENT_ALL, nullptr);
    lv_obj_add_state(editorArea, LV_STATE_FOCUSED);
}

void closeEditor() {
    if (editorOverlay == nullptr) {
        return;
    }

    if (acceptEditor && editorArea != nullptr) {
        const char* text = lv_textarea_get_text(editorArea);
        if (activeField == Field::Callsign) {
            copyText(callsignDraft, sizeof(callsignDraft), text);
            if (callsignArea != nullptr) {
                lv_textarea_set_text(callsignArea, callsignDraft);
            }
        } else if (activeField == Field::Latitude) {
            copyText(latitudeDraft, sizeof(latitudeDraft), text);
            if (latitudeArea != nullptr) {
                lv_textarea_set_text(latitudeArea, latitudeDraft);
            }
        } else if (activeField == Field::Longitude) {
            copyText(longitudeDraft, sizeof(longitudeDraft), text);
            if (longitudeArea != nullptr) {
                lv_textarea_set_text(longitudeArea, longitudeDraft);
            }
        }
    }

    lv_obj_del(editorOverlay);
    editorOverlay = nullptr;
    editorArea = nullptr;
    activeField = Field::None;
    pendingClose = false;
    acceptEditor = false;
}

bool parseCoordinate(const char* text, double& value) {
    if (text == nullptr || text[0] == '\0') {
        return false;
    }
    errno = 0;
    char* end = nullptr;
    const double parsed = std::strtod(text, &end);
    if (errno != 0 || end == text || end == nullptr || *end != '\0' || !std::isfinite(parsed)) {
        return false;
    }
    value = parsed;
    return true;
}

}  // namespace

void create(
    const Services::SettingsService::ViewState& state,
    App::SettingsSaveHandler handler,
    void* context) {

    saveHandler = handler;
    saveContext = context;
    copyText(callsignDraft, sizeof(callsignDraft), state.callsign);
    std::snprintf(latitudeDraft, sizeof(latitudeDraft), "%.6f", state.defaultLatitude);
    std::snprintf(longitudeDraft, sizeof(longitudeDraft), "%.6f", state.defaultLongitude);
    pendingOpen = Field::None;
    pendingClose = false;
    acceptEditor = false;
    activeField = Field::None;
    editorOverlay = nullptr;
    editorArea = nullptr;

    resetScreen();
    createHeader("Nastaveni APRS");

    lv_obj_t* card = lv_obj_create(lv_scr_act());
    lv_obj_set_size(card, 452, 190);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    createFieldRow(card, "CALL", callsignDraft, Field::Callsign, 2, callsignArea);
    createFieldRow(card, "Sirka", latitudeDraft, Field::Latitude, 47, latitudeArea);
    createFieldRow(card, "Delka", longitudeDraft, Field::Longitude, 92, longitudeArea);

    lv_obj_t* saveButton = lv_btn_create(card);
    lv_obj_set_size(saveButton, 132, 39);
    lv_obj_align(saveButton, LV_ALIGN_BOTTOM_RIGHT, -2, -2);
    lv_obj_set_style_bg_color(saveButton, lv_color_hex(0x2764D8), 0);
    lv_obj_add_event_cb(saveButton, saveClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* saveLabel = lv_label_create(saveButton);
    lv_label_set_text(saveLabel, "Ulozit");
    lv_obj_set_style_text_font(saveLabel, &lv_font_montserrat_16, 0);
    lv_obj_center(saveLabel);

    messageLabel = lv_label_create(card);
    lv_obj_set_width(messageLabel, 280);
    lv_label_set_long_mode(messageLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(
        messageLabel,
        state.persistentStorageReady ? "Hodnoty se ukladaji do NVS." : "NVS nebylo pri startu dostupne.");
    lv_obj_set_style_text_font(messageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(messageLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(messageLabel, LV_ALIGN_BOTTOM_LEFT, 4, -5);
}

void processPending() {
    if (pendingClose) {
        closeEditor();
        return;
    }
    if (pendingOpen != Field::None && editorOverlay == nullptr) {
        const Field field = pendingOpen;
        pendingOpen = Field::None;
        openEditor(field);
    }
}

void save() {
    if (saveHandler == nullptr || editorOverlay != nullptr) {
        return;
    }

    double latitude = 0.0;
    double longitude = 0.0;
    if (!parseCoordinate(latitudeDraft, latitude)) {
        setMessage("Neplatny format zemepisne sirky.");
        return;
    }
    if (!parseCoordinate(longitudeDraft, longitude)) {
        setMessage("Neplatny format zemepisne delky.");
        return;
    }

    char error[112] = {};
    const bool saved = saveHandler(
        callsignDraft,
        latitude,
        longitude,
        error,
        sizeof(error),
        saveContext);
    setMessage(saved ? "Nastaveni bylo ulozeno do NVS." : error);
}

void setMessage(const char* text) {
    if (messageLabel != nullptr) {
        lv_label_set_text(messageLabel, text != nullptr ? text : "");
        lv_obj_set_style_text_color(
            messageLabel,
            (text != nullptr && std::strstr(text, "ulozeno") != nullptr)
                ? lv_color_hex(0x42D392)
                : lv_color_hex(0xFFB454),
            0);
    }
}

}  // namespace SettingsScreen
}  // namespace Ui
