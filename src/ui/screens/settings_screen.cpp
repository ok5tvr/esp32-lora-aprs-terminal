#include "ui/screens/settings_screen.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <lvgl.h>

#include "app_config.h"
#include "app/localization.h"
#include "lora_profile.h"
#include "ui/ui_components.h"

namespace Ui {
namespace SettingsScreen {
namespace {

enum class Field : std::uint8_t {
    None,
    Callsign,
    Latitude,
    Longitude,
    LoRaFrequency
};

constexpr std::uint16_t timeoutValues[] = {0U, 30U, 60U, 120U, 300U};
constexpr float bandwidthValues[] = {62.5F, 125.0F, 250.0F, 500.0F};
constexpr char bandwidthOptions[] = "62.5 kHz\n125 kHz\n250 kHz\n500 kHz";
constexpr char spreadingFactorOptions[] = "SF7\nSF8\nSF9\nSF10\nSF11\nSF12";
constexpr char codingRateOptions[] = "4/5\n4/6\n4/7\n4/8";
constexpr std::int8_t powerValues[] = {2, 5, 10, 14, 17};
constexpr char powerOptions[] = "2 dBm\n5 dBm\n10 dBm\n14 dBm\n17 dBm";

App::SettingsSaveHandler saveHandler = nullptr;
void* saveContext = nullptr;

char callsignDraft[Services::SettingsService::CALLSIGN_CAPACITY] = {};
char latitudeDraft[24] = {};
char longitudeDraft[24] = {};
char frequencyDraft[16] = {};

lv_obj_t* callsignArea = nullptr;
lv_obj_t* latitudeArea = nullptr;
lv_obj_t* longitudeArea = nullptr;
lv_obj_t* frequencyArea = nullptr;
lv_obj_t* brightnessSlider = nullptr;
lv_obj_t* brightnessValueLabel = nullptr;
lv_obj_t* timeoutDropdown = nullptr;
lv_obj_t* uiLanguageDropdown = nullptr;
lv_obj_t* loraPresetDropdown = nullptr;
lv_obj_t* bandwidthDropdown = nullptr;
lv_obj_t* spreadingFactorDropdown = nullptr;
lv_obj_t* codingRateDropdown = nullptr;
lv_obj_t* powerDropdown = nullptr;
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

std::uint16_t timeoutFromIndex(std::uint16_t index) {
    const std::size_t count = sizeof(timeoutValues) / sizeof(timeoutValues[0]);
    return index < count ? timeoutValues[index] : AppConfig::DISPLAY_DEFAULT_TIMEOUT_SECONDS;
}

std::uint16_t indexFromTimeout(std::uint16_t timeoutSeconds) {
    const std::size_t count = sizeof(timeoutValues) / sizeof(timeoutValues[0]);
    for (std::size_t index = 0; index < count; ++index) {
        if (timeoutValues[index] == timeoutSeconds) {
            return static_cast<std::uint16_t>(index);
        }
    }
    return 2U;
}

std::uint16_t indexFromBandwidth(float value) {
    const std::size_t count = sizeof(bandwidthValues) / sizeof(bandwidthValues[0]);
    for (std::size_t index = 0; index < count; ++index) {
        if (std::fabs(value - bandwidthValues[index]) < 0.01F) {
            return static_cast<std::uint16_t>(index);
        }
    }
    return 1U;
}

float bandwidthFromIndex(std::uint16_t index) {
    const std::size_t count = sizeof(bandwidthValues) / sizeof(bandwidthValues[0]);
    return index < count ? bandwidthValues[index] : LoRaProfile::BANDWIDTH_KHZ;
}

std::uint16_t indexFromPower(std::int8_t value) {
    const std::size_t count = sizeof(powerValues) / sizeof(powerValues[0]);
    for (std::size_t index = 0; index < count; ++index) {
        if (powerValues[index] == value) {
            return static_cast<std::uint16_t>(index);
        }
    }
    return 2U;
}

std::int8_t powerFromIndex(std::uint16_t index) {
    const std::size_t count = sizeof(powerValues) / sizeof(powerValues[0]);
    return index < count ? powerValues[index] : LoRaProfile::OUTPUT_POWER_DBM;
}

void updateBrightnessLabel() {
    if (brightnessSlider != nullptr && brightnessValueLabel != nullptr) {
        lv_label_set_text_fmt(
            brightnessValueLabel,
            "%d %%",
            static_cast<int>(lv_slider_get_value(brightnessSlider)));
    }
}

void brightnessChanged(lv_event_t* event) {
    if (lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED) {
        updateBrightnessLabel();
    }
}

void fieldClicked(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || editorOverlay != nullptr) {
        return;
    }
    pendingOpen = static_cast<Field>(
        reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
}

void setCustomControlsEnabled(bool enabled) {
    lv_obj_t* controls[] = {
        frequencyArea,
        bandwidthDropdown,
        spreadingFactorDropdown,
        codingRateDropdown,
        powerDropdown
    };
    for (lv_obj_t* control : controls) {
        if (control == nullptr) {
            continue;
        }
        if (enabled) {
            lv_obj_clear_state(control, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(control, LV_STATE_DISABLED);
        }
    }
}

void loadCzePresetIntoControls() {
    std::snprintf(frequencyDraft, sizeof(frequencyDraft), "%.3f", LoRaProfile::FREQUENCY_MHZ);
    if (frequencyArea != nullptr) {
        lv_textarea_set_text(frequencyArea, frequencyDraft);
    }
    if (bandwidthDropdown != nullptr) {
        lv_dropdown_set_selected(bandwidthDropdown, indexFromBandwidth(LoRaProfile::BANDWIDTH_KHZ));
    }
    if (spreadingFactorDropdown != nullptr) {
        lv_dropdown_set_selected(
            spreadingFactorDropdown,
            LoRaProfile::SPREADING_FACTOR - LoRaProfile::MIN_SPREADING_FACTOR);
    }
    if (codingRateDropdown != nullptr) {
        lv_dropdown_set_selected(
            codingRateDropdown,
            LoRaProfile::CODING_RATE - LoRaProfile::MIN_CODING_RATE);
    }
    if (powerDropdown != nullptr) {
        lv_dropdown_set_selected(powerDropdown, indexFromPower(LoRaProfile::OUTPUT_POWER_DBM));
    }
}

void applyPresetSelection(bool loadDefaults) {
    if (loraPresetDropdown == nullptr) {
        return;
    }
    const bool custom = lv_dropdown_get_selected(loraPresetDropdown) ==
        static_cast<std::uint16_t>(App::LoRaPreset::Custom);
    if (!custom && loadDefaults) {
        loadCzePresetIntoControls();
    }
    setCustomControlsEnabled(custom);
}

void profileChanged(lv_event_t* event) {
    if (lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED) {
        applyPresetSelection(true);
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

lv_obj_t* createDropdownRow(
    lv_obj_t* parent,
    const char* labelText,
    const char* options,
    std::uint16_t selected,
    lv_coord_t y) {

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, labelText);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 4, y + 9);

    lv_obj_t* dropdown = lv_dropdown_create(parent);
    lv_obj_set_size(dropdown, 270, 40);
    lv_obj_align(dropdown, LV_ALIGN_TOP_RIGHT, -2, y);
    lv_dropdown_set_options(dropdown, options);
    lv_dropdown_set_selected(dropdown, selected);
    lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_16, 0);
    return dropdown;
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
    const char* titleText = App::Localization::text("Editace", "Edit");
    if (field == Field::Callsign) {
        titleText = App::Localization::text("Editace CALL", "Edit callsign");
    } else if (field == Field::Latitude) {
        titleText = App::Localization::text("Vychozi zemepisna sirka", "Default latitude");
    } else if (field == Field::Longitude) {
        titleText = App::Localization::text("Vychozi zemepisna delka", "Default longitude");
    } else if (field == Field::LoRaFrequency) {
        titleText = App::Localization::text("LoRa frekvence v MHz", "LoRa frequency in MHz");
    }
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
    } else if (field == Field::Longitude) {
        lv_textarea_set_text(editorArea, longitudeDraft);
        lv_textarea_set_max_length(editorArea, sizeof(longitudeDraft) - 1);
        lv_textarea_set_accepted_chars(editorArea, "0123456789.-");
    } else {
        lv_textarea_set_text(editorArea, frequencyDraft);
        lv_textarea_set_max_length(editorArea, sizeof(frequencyDraft) - 1);
        lv_textarea_set_accepted_chars(editorArea, "0123456789.");
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
            if (callsignArea != nullptr) lv_textarea_set_text(callsignArea, callsignDraft);
        } else if (activeField == Field::Latitude) {
            copyText(latitudeDraft, sizeof(latitudeDraft), text);
            if (latitudeArea != nullptr) lv_textarea_set_text(latitudeArea, latitudeDraft);
        } else if (activeField == Field::Longitude) {
            copyText(longitudeDraft, sizeof(longitudeDraft), text);
            if (longitudeArea != nullptr) lv_textarea_set_text(longitudeArea, longitudeDraft);
        } else if (activeField == Field::LoRaFrequency) {
            copyText(frequencyDraft, sizeof(frequencyDraft), text);
            if (frequencyArea != nullptr) lv_textarea_set_text(frequencyArea, frequencyDraft);
        }
    }

    lv_obj_del(editorOverlay);
    editorOverlay = nullptr;
    editorArea = nullptr;
    activeField = Field::None;
    pendingClose = false;
    acceptEditor = false;
}

bool parseNumber(const char* text, double& value) {
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
    std::snprintf(frequencyDraft, sizeof(frequencyDraft), "%.3f", state.loraFrequencyMHz);
    pendingOpen = Field::None;
    pendingClose = false;
    acceptEditor = false;
    activeField = Field::None;
    editorOverlay = nullptr;
    editorArea = nullptr;

    resetScreen();
    createHeader(App::Localization::text("Nastaveni", "Settings"));

    lv_obj_t* card = lv_obj_create(lv_scr_act());
    lv_obj_set_size(card, 452, 196);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_scroll_dir(card, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_AUTO);

    createFieldRow(card, "CALL", callsignDraft, Field::Callsign, 2, callsignArea);
    createFieldRow(card, App::Localization::text("Sirka", "Latitude"), latitudeDraft, Field::Latitude, 47, latitudeArea);
    createFieldRow(card, App::Localization::text("Delka", "Longitude"), longitudeDraft, Field::Longitude, 92, longitudeArea);

    uiLanguageDropdown = createDropdownRow(
        card,
        App::Localization::text("Jazyk rozhrani", "Interface language"),
        App::Localization::text("Cestina\nEnglish", "Czech\nEnglish"),
        static_cast<std::uint16_t>(state.uiLanguage),
        137);

    lv_obj_t* brightnessLabel = lv_label_create(card);
    lv_label_set_text(brightnessLabel, App::Localization::text("Jas z baterie", "Battery brightness"));
    lv_obj_set_style_text_font(brightnessLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(brightnessLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(brightnessLabel, LV_ALIGN_TOP_LEFT, 4, 188);

    brightnessValueLabel = lv_label_create(card);
    lv_obj_set_style_text_font(brightnessValueLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(brightnessValueLabel, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(brightnessValueLabel, LV_ALIGN_TOP_RIGHT, -4, 188);

    brightnessSlider = lv_slider_create(card);
    lv_obj_set_size(brightnessSlider, 420, 18);
    lv_obj_align(brightnessSlider, LV_ALIGN_TOP_MID, 0, 215);
    lv_slider_set_range(
        brightnessSlider,
        AppConfig::DISPLAY_MIN_BATTERY_BRIGHTNESS_PERCENT,
        AppConfig::DISPLAY_MAX_BATTERY_BRIGHTNESS_PERCENT);
    lv_slider_set_value(brightnessSlider, state.batteryBrightnessPercent, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightnessSlider, brightnessChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    updateBrightnessLabel();

    lv_obj_t* timeoutLabel = lv_label_create(card);
    lv_label_set_text(timeoutLabel, App::Localization::text("Vypnout displej", "Turn display off"));
    lv_obj_set_style_text_font(timeoutLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(timeoutLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(timeoutLabel, LV_ALIGN_TOP_LEFT, 4, 253);

    timeoutDropdown = lv_dropdown_create(card);
    lv_obj_set_size(timeoutDropdown, 220, 40);
    lv_obj_align(timeoutDropdown, LV_ALIGN_TOP_RIGHT, -2, 243);
    lv_dropdown_set_options(
        timeoutDropdown,
        App::Localization::text("Nikdy\n30 s\n60 s\n2 min\n5 min", "Never\n30 s\n60 s\n2 min\n5 min"));
    lv_dropdown_set_selected(timeoutDropdown, indexFromTimeout(state.displayTimeoutSeconds));
    lv_obj_set_style_text_font(timeoutDropdown, &lv_font_montserrat_16, 0);

    lv_obj_t* displayInfo = lv_label_create(card);
    lv_label_set_text(
        displayInfo,
        App::Localization::text(
            "Baterie: po 30 s jas 15 %, pak vypnout dle volby",
            "Battery: dim to 15% after 30 s, then turn off as selected"));
    lv_obj_set_width(displayInfo, 420);
    lv_label_set_long_mode(displayInfo, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(displayInfo, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(displayInfo, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(displayInfo, LV_ALIGN_TOP_LEFT, 4, 293);

    lv_obj_t* divider = lv_obj_create(card);
    lv_obj_set_size(divider, 420, 1);
    lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, 332);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);

    lv_obj_t* loraTitle = lv_label_create(card);
    lv_label_set_text(loraTitle, App::Localization::text("LoRa modul", "LoRa module"));
    lv_obj_set_style_text_font(loraTitle, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(loraTitle, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(loraTitle, LV_ALIGN_TOP_LEFT, 4, 344);

    loraPresetDropdown = createDropdownRow(
        card,
        App::Localization::text("Profil", "Profile"),
        App::Localization::text("CZE APRS\nVlastni", "CZE APRS\nCustom"),
        static_cast<std::uint16_t>(state.loraPreset),
        371);
    lv_obj_add_event_cb(loraPresetDropdown, profileChanged, LV_EVENT_VALUE_CHANGED, nullptr);

    createFieldRow(
        card,
        App::Localization::text("Frekvence", "Frequency"),
        frequencyDraft,
        Field::LoRaFrequency,
        416,
        frequencyArea);

    bandwidthDropdown = createDropdownRow(
        card,
        App::Localization::text("Sirka pasma", "Bandwidth"),
        bandwidthOptions,
        indexFromBandwidth(state.loraBandwidthKHz),
        461);
    spreadingFactorDropdown = createDropdownRow(
        card,
        App::Localization::text("Rozprostiraci faktor", "Spreading factor"),
        spreadingFactorOptions,
        state.loraSpreadingFactor - LoRaProfile::MIN_SPREADING_FACTOR,
        506);
    codingRateDropdown = createDropdownRow(
        card,
        App::Localization::text("Kodovy pomer", "Coding rate"),
        codingRateOptions,
        state.loraCodingRate - LoRaProfile::MIN_CODING_RATE,
        551);
    powerDropdown = createDropdownRow(
        card,
        App::Localization::text("TX vykon", "TX power"),
        powerOptions,
        indexFromPower(state.loraOutputPowerDbm),
        596);

    lv_obj_t* loraInfo = lv_label_create(card);
    lv_label_set_text(
        loraInfo,
        App::Localization::text(
            "CZE: 433.775 MHz, BW125, SF12, CR4/5, 10 dBm. Sync 0x12 a CRC zustavaji pevne.",
            "CZE: 433.775 MHz, BW125, SF12, CR4/5, 10 dBm. Sync 0x12 and CRC remain fixed."));
    lv_obj_set_width(loraInfo, 420);
    lv_label_set_long_mode(loraInfo, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(loraInfo, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(loraInfo, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(loraInfo, LV_ALIGN_TOP_LEFT, 4, 641);

    lv_obj_t* versionLabel = lv_label_create(card);
    lv_label_set_text_fmt(
        versionLabel,
        "Firmware: %s v%s",
        AppConfig::FIRMWARE_NAME,
        AppConfig::FIRMWARE_VERSION);
    lv_obj_set_style_text_font(versionLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(versionLabel, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(versionLabel, LV_ALIGN_TOP_LEFT, 4, 688);

    lv_obj_t* saveButton = lv_btn_create(card);
    lv_obj_set_size(saveButton, 132, 39);
    lv_obj_align(saveButton, LV_ALIGN_TOP_RIGHT, -2, 711);
    lv_obj_set_style_bg_color(saveButton, lv_color_hex(0x2764D8), 0);
    lv_obj_add_event_cb(saveButton, saveClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* saveLabel = lv_label_create(saveButton);
    lv_label_set_text(saveLabel, App::Localization::text("Ulozit", "Save"));
    lv_obj_set_style_text_font(saveLabel, &lv_font_montserrat_16, 0);
    lv_obj_center(saveLabel);

    messageLabel = lv_label_create(card);
    lv_obj_set_width(messageLabel, 280);
    lv_label_set_long_mode(messageLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(
        messageLabel,
        state.persistentStorageReady
            ? App::Localization::text("NVS: aktivni", "NVS: active")
            : App::Localization::text("NVS: nedostupne", "NVS: unavailable"));
    lv_obj_set_style_text_font(messageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(messageLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(messageLabel, LV_ALIGN_TOP_LEFT, 4, 719);

    applyPresetSelection(false);
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
    if (saveHandler == nullptr || editorOverlay != nullptr ||
        brightnessSlider == nullptr || timeoutDropdown == nullptr ||
        uiLanguageDropdown == nullptr || loraPresetDropdown == nullptr ||
        bandwidthDropdown == nullptr ||
        spreadingFactorDropdown == nullptr || codingRateDropdown == nullptr ||
        powerDropdown == nullptr) {
        return;
    }

    double latitude = 0.0;
    double longitude = 0.0;
    if (!parseNumber(latitudeDraft, latitude)) {
        setMessage(App::Localization::text("Neplatny format zemepisne sirky.", "Invalid latitude format."));
        return;
    }
    if (!parseNumber(longitudeDraft, longitude)) {
        setMessage(App::Localization::text("Neplatny format zemepisne delky.", "Invalid longitude format."));
        return;
    }

    const App::LoRaPreset preset = static_cast<App::LoRaPreset>(
        lv_dropdown_get_selected(loraPresetDropdown));
    LoRaProfile::Config config = LoRaProfile::czeAprsConfig();
    if (preset == App::LoRaPreset::Custom) {
        double frequency = 0.0;
        if (!parseNumber(frequencyDraft, frequency)) {
            setMessage(App::Localization::text("Neplatna frekvence LoRa.", "Invalid LoRa frequency."));
            return;
        }
        config.frequencyMHz = static_cast<float>(frequency);
        config.bandwidthKHz = bandwidthFromIndex(
            lv_dropdown_get_selected(bandwidthDropdown));
        config.spreadingFactor = static_cast<std::uint8_t>(
            LoRaProfile::MIN_SPREADING_FACTOR +
            lv_dropdown_get_selected(spreadingFactorDropdown));
        config.codingRate = static_cast<std::uint8_t>(
            LoRaProfile::MIN_CODING_RATE +
            lv_dropdown_get_selected(codingRateDropdown));
        config.outputPowerDbm = powerFromIndex(
            lv_dropdown_get_selected(powerDropdown));
    }

    const std::uint8_t brightness = static_cast<std::uint8_t>(
        lv_slider_get_value(brightnessSlider));
    const std::uint16_t timeout = timeoutFromIndex(
        lv_dropdown_get_selected(timeoutDropdown));
    const App::UiLanguage uiLanguage = static_cast<App::UiLanguage>(
        lv_dropdown_get_selected(uiLanguageDropdown));

    char error[160] = {};
    const bool saved = saveHandler(
        callsignDraft,
        latitude,
        longitude,
        brightness,
        timeout,
        uiLanguage,
        preset,
        config.frequencyMHz,
        config.bandwidthKHz,
        config.spreadingFactor,
        config.codingRate,
        config.outputPowerDbm,
        error,
        sizeof(error),
        saveContext);
    setMessage(
        saved
            ? App::Localization::text(
                "Nastaveni ulozeno; LoRa se prepne po uvolneni TX fronty.",
                "Settings saved; LoRa will switch after the TX queue is empty.")
            : error);
}

void setMessage(const char* text) {
    if (messageLabel != nullptr) {
        lv_label_set_text(messageLabel, text != nullptr ? text : "");
        lv_obj_set_style_text_color(
            messageLabel,
            (text != nullptr &&
             (std::strstr(text, "ulozeno") != nullptr ||
              std::strstr(text, "saved") != nullptr))
                ? lv_color_hex(0x42D392)
                : lv_color_hex(0xFFB454),
            0);
    }
}

}  // namespace SettingsScreen
}  // namespace Ui
