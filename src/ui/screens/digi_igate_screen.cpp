#include "ui/screens/digi_igate_screen.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <lvgl.h>

#include "ui/ui_components.h"

namespace Ui {
namespace DigiIgateScreen {
namespace {

enum class Field : std::uint8_t {
    None,
    DigiEnabled,
    DigiMaxWide,
    IgateEnabled,
    WifiSsid,
    WifiPassword,
    Server,
    Port,
    Passcode,
    Filter
};

constexpr const char* DIGI_MODE_OPTIONS =
    "WIDE1-1 fill-in\n"
    "WIDE2-N trace\n"
    "WIDE1 + WIDE2";

App::DigiIgateSettingsSaveHandler currentSaveHandler = nullptr;
void* currentSaveContext = nullptr;
lv_obj_t* content = nullptr;
lv_obj_t* statusLabel = nullptr;
lv_obj_t* networkLabel = nullptr;
lv_obj_t* messageLabel = nullptr;
lv_obj_t* modeDropdown = nullptr;
lv_obj_t* valueLabels[3] = {};
lv_obj_t* textAreas[6] = {};
lv_obj_t* editorOverlay = nullptr;
lv_obj_t* editorArea = nullptr;
Field activeField = Field::None;
Field pendingOpen = Field::None;
bool pendingClose = false;
bool acceptEditor = false;

bool draftDigiEnabled = false;
App::DigiMode draftDigiMode = App::DigiMode::FillInWide1;
std::uint8_t draftMaxWide = 2;
bool draftIgateEnabled = false;
char draftSsid[Services::SettingsService::WIFI_SSID_CAPACITY] = {};
char draftPassword[Services::SettingsService::WIFI_PASSWORD_CAPACITY] = {};
char draftServer[Services::SettingsService::APRS_IS_SERVER_CAPACITY] = {};
char draftPort[8] = {};
char draftPasscode[12] = {};
char draftFilter[Services::SettingsService::APRS_IS_FILTER_CAPACITY] = {};
std::uint32_t observedSettingsRevision = 0xFFFFFFFFU;

void copyText(char* output, std::size_t capacity, const char* input) {
    if (output == nullptr || capacity == 0) {
        return;
    }
    std::snprintf(output, capacity, "%s", input != nullptr ? input : "");
}

const char* draftForField(Field field) {
    switch (field) {
        case Field::WifiSsid: return draftSsid;
        case Field::WifiPassword: return draftPassword;
        case Field::Server: return draftServer;
        case Field::Port: return draftPort;
        case Field::Passcode: return draftPasscode;
        case Field::Filter: return draftFilter;
        default: return "";
    }
}

std::size_t capacityForField(Field field) {
    switch (field) {
        case Field::WifiSsid: return sizeof(draftSsid);
        case Field::WifiPassword: return sizeof(draftPassword);
        case Field::Server: return sizeof(draftServer);
        case Field::Port: return sizeof(draftPort);
        case Field::Passcode: return sizeof(draftPasscode);
        case Field::Filter: return sizeof(draftFilter);
        default: return 1;
    }
}

char* mutableDraftForField(Field field) {
    switch (field) {
        case Field::WifiSsid: return draftSsid;
        case Field::WifiPassword: return draftPassword;
        case Field::Server: return draftServer;
        case Field::Port: return draftPort;
        case Field::Passcode: return draftPasscode;
        case Field::Filter: return draftFilter;
        default: return nullptr;
    }
}

std::size_t areaIndex(Field field) {
    switch (field) {
        case Field::WifiSsid: return 0;
        case Field::WifiPassword: return 1;
        case Field::Server: return 2;
        case Field::Port: return 3;
        case Field::Passcode: return 4;
        case Field::Filter: return 5;
        default: return 0;
    }
}

void refreshDraftLabels() {
    if (valueLabels[0] != nullptr) {
        lv_label_set_text(valueLabels[0], draftDigiEnabled ? "ZAPNUT" : "VYPNUT");
    }
    if (valueLabels[1] != nullptr) {
        char text[16] = {};
        std::snprintf(text, sizeof(text), "%u hop", static_cast<unsigned>(draftMaxWide));
        lv_label_set_text(valueLabels[1], text);
    }
    if (valueLabels[2] != nullptr) {
        lv_label_set_text(valueLabels[2], draftIgateEnabled ? "ZAPNUTA" : "VYPNUTA");
    }
    if (modeDropdown != nullptr) {
        lv_dropdown_set_selected(
            modeDropdown,
            static_cast<std::uint16_t>(draftDigiMode));
    }
    for (std::size_t index = 0; index < 6; ++index) {
        if (textAreas[index] == nullptr) {
            continue;
        }
        const Field field = static_cast<Field>(
            static_cast<std::uint8_t>(Field::WifiSsid) + index);
        if (field == Field::WifiPassword) {
            lv_textarea_set_text(textAreas[index], draftPassword[0] != '\0' ? "********" : "");
        } else {
            lv_textarea_set_text(textAreas[index], draftForField(field));
        }
    }
}

void copyFromSettings(const Services::SettingsService::ViewState& settings) {
    draftDigiEnabled = settings.digiEnabled;
    draftDigiMode = settings.digiMode;
    draftMaxWide = settings.digiMaxWideHops;
    draftIgateEnabled = settings.igateEnabled;
    copyText(draftSsid, sizeof(draftSsid), settings.wifiSsid);
    copyText(draftPassword, sizeof(draftPassword), settings.wifiPassword);
    copyText(draftServer, sizeof(draftServer), settings.aprsIsServer);
    std::snprintf(draftPort, sizeof(draftPort), "%u", static_cast<unsigned>(settings.aprsIsPort));
    std::snprintf(draftPasscode, sizeof(draftPasscode), "%ld", static_cast<long>(settings.aprsIsPasscode));
    copyText(draftFilter, sizeof(draftFilter), settings.aprsIsFilter);
    observedSettingsRevision = settings.revision;
    refreshDraftLabels();
}

void fieldClicked(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || editorOverlay != nullptr) {
        return;
    }
    const Field field = static_cast<Field>(
        reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
    switch (field) {
        case Field::DigiEnabled:
            draftDigiEnabled = !draftDigiEnabled;
            refreshDraftLabels();
            break;
        case Field::DigiMaxWide:
            draftMaxWide = draftMaxWide == 1 ? 2 : 1;
            refreshDraftLabels();
            break;
        case Field::IgateEnabled:
            draftIgateEnabled = !draftIgateEnabled;
            refreshDraftLabels();
            break;
        default:
            pendingOpen = field;
            break;
    }
}

void modeChanged(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED || modeDropdown == nullptr) {
        return;
    }
    const std::uint16_t selected = lv_dropdown_get_selected(modeDropdown);
    if (selected <= static_cast<std::uint16_t>(App::DigiMode::FillInAndWide2)) {
        draftDigiMode = static_cast<App::DigiMode>(selected);
    }
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

void createToggleRow(
    const char* title,
    const char* hint,
    Field field,
    std::size_t valueIndex) {

    lv_obj_t* row = lv_obj_create(content);
    lv_obj_set_size(row, 438, 44);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 9, 0);
    lv_obj_set_style_pad_left(row, 10, 0);
    lv_obj_set_style_pad_right(row, 10, 0);
    lv_obj_set_style_pad_top(row, 0, 0);
    lv_obj_set_style_pad_bottom(row, 0, 0);
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
    lv_obj_align(hintLabel, LV_ALIGN_BOTTOM_LEFT, 0, -5);

    valueLabels[valueIndex] = lv_label_create(row);
    lv_obj_set_style_text_font(valueLabels[valueIndex], &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(valueLabels[valueIndex], lv_color_hex(0x56C7FF), 0);
    lv_obj_align(valueLabels[valueIndex], LV_ALIGN_RIGHT_MID, 0, 0);
}

void createModeRow() {
    lv_obj_t* row = lv_obj_create(content);
    lv_obj_set_size(row, 438, 48);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 9, 0);
    lv_obj_set_style_pad_left(row, 10, 0);
    lv_obj_set_style_pad_right(row, 8, 0);
    lv_obj_set_style_pad_top(row, 0, 0);
    lv_obj_set_style_pad_bottom(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, "Rezim DIGI");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    modeDropdown = lv_dropdown_create(row);
    lv_obj_set_size(modeDropdown, 220, 36);
    lv_obj_align(modeDropdown, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(modeDropdown, &lv_font_montserrat_14, 0);
    lv_dropdown_set_options(modeDropdown, DIGI_MODE_OPTIONS);
    lv_dropdown_set_selected(modeDropdown, static_cast<std::uint16_t>(draftDigiMode));
    lv_obj_add_event_cb(modeDropdown, modeChanged, LV_EVENT_VALUE_CHANGED, nullptr);
}

void createTextRow(const char* title, Field field, const char* hint) {
    lv_obj_t* row = lv_obj_create(content);
    lv_obj_set_size(row, 438, 49);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 9, 0);
    lv_obj_set_style_pad_left(row, 10, 0);
    lv_obj_set_style_pad_right(row, 8, 0);
    lv_obj_set_style_pad_top(row, 0, 0);
    lv_obj_set_style_pad_bottom(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 1);

    lv_obj_t* hintLabel = lv_label_create(row);
    lv_label_set_text(hintLabel, hint);
    lv_obj_set_style_text_font(hintLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hintLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(hintLabel, LV_ALIGN_BOTTOM_LEFT, 0, -5);

    lv_obj_t* area = lv_textarea_create(row);
    textAreas[areaIndex(field)] = area;
    lv_obj_set_size(area, 250, 37);
    lv_obj_align(area, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_textarea_set_one_line(area, true);
    lv_textarea_set_text(area, field == Field::WifiPassword && draftPassword[0] != '\0'
        ? "********"
        : draftForField(field));
    lv_obj_set_style_text_font(area, &lv_font_montserrat_14, 0);
    lv_obj_add_event_cb(
        area,
        fieldClicked,
        LV_EVENT_CLICKED,
        reinterpret_cast<void*>(static_cast<std::intptr_t>(field)));
}

void openEditor(Field field) {
    if (field < Field::WifiSsid || field > Field::Filter || editorOverlay != nullptr) {
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

    const char* title = "Editace";
    switch (field) {
        case Field::WifiSsid: title = "WiFi SSID"; break;
        case Field::WifiPassword: title = "WiFi heslo"; break;
        case Field::Server: title = "APRS-IS server"; break;
        case Field::Port: title = "APRS-IS port"; break;
        case Field::Passcode: title = "APRS-IS passcode"; break;
        case Field::Filter: title = "APRS-IS filter"; break;
        default: break;
    }
    lv_obj_t* titleLabel = lv_label_create(editorOverlay);
    lv_label_set_text(titleLabel, title);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 14, 8);

    editorArea = lv_textarea_create(editorOverlay);
    lv_obj_set_size(editorArea, 452, 43);
    lv_obj_align(editorArea, LV_ALIGN_TOP_MID, 0, 38);
    lv_textarea_set_one_line(editorArea, true);
    lv_textarea_set_text(editorArea, draftForField(field));
    lv_textarea_set_max_length(editorArea, capacityForField(field) - 1);
    lv_obj_set_style_text_font(editorArea, &lv_font_montserrat_18, 0);
    if (field == Field::Port) {
        lv_textarea_set_accepted_chars(editorArea, "0123456789");
    } else if (field == Field::Passcode) {
        lv_textarea_set_accepted_chars(editorArea, "0123456789-");
    } else if (field == Field::Server) {
        lv_textarea_set_accepted_chars(
            editorArea,
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-");
    }
    if (field == Field::WifiPassword) {
        lv_textarea_set_password_mode(editorArea, true);
        lv_textarea_set_password_show_time(editorArea, 500);
    }

    lv_obj_t* hint = lv_label_create(editorOverlay);
    lv_label_set_text(
        hint,
        field == Field::Passcode
            ? "Pro aktivni iGate je nutny platny APRS-IS passcode; -1 je jen neovereny login."
            : (field == Field::Filter
                ? "Zadejte pouze vyraz filtru, napr. r/49.78/13.28/50; lze ponechat prazdne."
                : "Potvrdte tlacitkem klavesnice."));
    lv_obj_set_width(hint, 452);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 14, 85);

    lv_obj_t* keyboard = lv_keyboard_create(editorOverlay);
    const bool numeric = field == Field::Port || field == Field::Passcode;
    lv_obj_set_size(keyboard, 468, numeric ? 210 : 190);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_keyboard_set_textarea(keyboard, editorArea);
    lv_keyboard_set_mode(
        keyboard,
        numeric ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_event_cb(keyboard, keyboardEvent, LV_EVENT_ALL, nullptr);
    lv_obj_add_state(editorArea, LV_STATE_FOCUSED);
}

void closeEditor() {
    if (editorOverlay == nullptr) {
        return;
    }
    if (acceptEditor && editorArea != nullptr) {
        char* destination = mutableDraftForField(activeField);
        if (destination != nullptr) {
            copyText(
                destination,
                capacityForField(activeField),
                lv_textarea_get_text(editorArea));
        }
    }
    lv_obj_del(editorOverlay);
    editorOverlay = nullptr;
    editorArea = nullptr;
    activeField = Field::None;
    pendingClose = false;
    acceptEditor = false;
    refreshDraftLabels();
}

bool parseUnsigned(const char* text, unsigned long maximum, unsigned long& value) {
    if (text == nullptr || text[0] == '\0') {
        return false;
    }
    errno = 0;
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed > maximum) {
        return false;
    }
    value = parsed;
    return true;
}

bool parseSigned(const char* text, long minimum, long maximum, long& value) {
    if (text == nullptr || text[0] == '\0') {
        return false;
    }
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed < minimum || parsed > maximum) {
        return false;
    }
    value = parsed;
    return true;
}

}  // namespace

void create(
    const Services::SettingsService::ViewState& settings,
    const Services::DigiIgateService::ViewState& state,
    App::DigiIgateSettingsSaveHandler saveHandler,
    void* saveContext) {

    currentSaveHandler = saveHandler;
    currentSaveContext = saveContext;
    content = nullptr;
    statusLabel = nullptr;
    networkLabel = nullptr;
    messageLabel = nullptr;
    modeDropdown = nullptr;
    editorOverlay = nullptr;
    editorArea = nullptr;
    activeField = Field::None;
    pendingOpen = Field::None;
    pendingClose = false;
    acceptEditor = false;
    std::memset(valueLabels, 0, sizeof(valueLabels));
    std::memset(textAreas, 0, sizeof(textAreas));

    resetScreen();
    createHeader("APRS DIGI / iGate");

    statusLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(statusLabel, 452);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 37);

    networkLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(networkLabel, 452);
    lv_label_set_long_mode(networkLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(networkLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(networkLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(networkLabel, LV_ALIGN_TOP_MID, 0, 54);

    content = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, 452, 166);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 74);
    lv_obj_set_style_pad_left(content, 5, 0);
    lv_obj_set_style_pad_right(content, 5, 0);
    lv_obj_set_style_pad_top(content, 2, 0);
    lv_obj_set_style_pad_bottom(content, 2, 0);
    lv_obj_set_style_pad_row(content, 5, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);

    copyFromSettings(settings);
    createToggleRow("Digipeater (RF -> RF)", "Opakuje povolene WIDE pakety", Field::DigiEnabled, 0);
    createModeRow();
    createToggleRow("Max. WIDE", "Hodnoty nad limitem se neopakuji", Field::DigiMaxWide, 1);
    createToggleRow("RX iGate (RF -> IS)", "Bez prenosu z internetu zpet na RF", Field::IgateEnabled, 2);
    createTextRow("WiFi SSID", Field::WifiSsid, "Sit pro APRS-IS");
    createTextRow("WiFi heslo", Field::WifiPassword, "Ulozeno v NVS");
    createTextRow("APRS-IS server", Field::Server, "Vychozi rotate.aprs2.net");
    createTextRow("Port", Field::Port, "Obvykle 14580");
    createTextRow("Passcode", Field::Passcode, "Musi byt overeny");
    createTextRow("Filter", Field::Filter, "Volitelny serverovy filter");

    lv_obj_t* saveButton = lv_btn_create(content);
    lv_obj_set_size(saveButton, 438, 40);
    lv_obj_set_style_bg_color(saveButton, lv_color_hex(0x2764D8), 0);
    lv_obj_add_event_cb(saveButton, saveClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* saveLabel = lv_label_create(saveButton);
    lv_label_set_text(saveLabel, "Ulozit a pouzit DIGI/iGate");
    lv_obj_set_style_text_font(saveLabel, &lv_font_montserrat_16, 0);
    lv_obj_center(saveLabel);

    messageLabel = lv_label_create(content);
    lv_obj_set_width(messageLabel, 430);
    lv_label_set_long_mode(messageLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(
        messageLabel,
        "Sluzby jsou nezavisle: jen RX iGate = DIGI VYP / iGate ZAP; jen DIGI = DIGI ZAP / iGate VYP.");
    lv_obj_set_style_text_font(messageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(messageLabel, lv_color_hex(0x92A7C7), 0);

    refreshDraftLabels();
    update(state, settings);
}

void update(
    const Services::DigiIgateService::ViewState& state,
    const Services::SettingsService::ViewState& settings) {

    if (settings.revision != observedSettingsRevision && editorOverlay == nullptr) {
        copyFromSettings(settings);
    }
    if (statusLabel != nullptr) {
        lv_label_set_text(statusLabel, state.statusText);
        lv_obj_set_style_text_color(
            statusLabel,
            state.digiEnabled ? lv_color_hex(0x42D392) : lv_color_hex(0x92A7C7),
            0);
    }
    if (networkLabel != nullptr) {
        char text[160] = {};
        std::snprintf(
            text,
            sizeof(text),
            "WiFi %s%s%s | IS %s | gate %lu filt %lu q%u",
            state.wifiConnected ? "OK" : "--",
            state.wifiConnected ? " " : "",
            state.wifiConnected ? state.wifiAddress : "",
            state.aprsIsVerified
                ? "VERIFIED"
                : (state.loginRejected ? "REJECT" : (state.aprsIsConnected ? "LOGIN" : "--")),
            static_cast<unsigned long>(state.gatedPackets),
            static_cast<unsigned long>(state.gateFiltered),
            static_cast<unsigned>(state.igateQueueDepth));
        lv_label_set_text(networkLabel, text);
        lv_obj_set_style_text_color(
            networkLabel,
            state.aprsIsVerified
                ? lv_color_hex(0x42D392)
                : (state.loginRejected ? lv_color_hex(0xFF6B6B) : lv_color_hex(0xFFB454)),
            0);
    }
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
    if (currentSaveHandler == nullptr || editorOverlay != nullptr) {
        return;
    }
    unsigned long port = 0;
    long passcode = 0;
    if (!parseUnsigned(draftPort, 65535, port) || port == 0) {
        setMessage("Neplatny APRS-IS port.");
        return;
    }
    if (!parseSigned(draftPasscode, -1, 32767, passcode)) {
        setMessage("Neplatny APRS-IS passcode.");
        return;
    }

    char error[160] = {};
    const bool saved = currentSaveHandler(
        draftDigiEnabled,
        draftDigiMode,
        draftMaxWide,
        draftIgateEnabled,
        draftSsid,
        draftPassword,
        draftServer,
        static_cast<std::uint16_t>(port),
        static_cast<std::int32_t>(passcode),
        draftFilter,
        error,
        sizeof(error),
        currentSaveContext);
    setMessage(saved ? "Nastaveni ulozeno; sluzby byly aktualizovany." : error);
}

void scroll(int direction) {
    if (content == nullptr || direction == 0) {
        return;
    }
    lv_obj_scroll_by(content, 0, direction > 0 ? -52 : 52, LV_ANIM_ON);
}

void setMessage(const char* text) {
    if (messageLabel == nullptr) {
        return;
    }
    lv_label_set_text(messageLabel, text != nullptr ? text : "");
    lv_obj_set_style_text_color(
        messageLabel,
        text != nullptr && std::strstr(text, "ulozeno") != nullptr
            ? lv_color_hex(0x42D392)
            : lv_color_hex(0xFFB454),
        0);
}

}  // namespace DigiIgateScreen
}  // namespace Ui
