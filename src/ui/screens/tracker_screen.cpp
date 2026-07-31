#include "ui/screens/tracker_screen.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "app/tracker_symbols.h"
#include "ui/ui_components.h"

namespace Ui {
namespace TrackerScreen {
namespace {

enum class Field : std::uint8_t {
    Enabled,
    TrailEnabled,
    Source,
    Format,
    Mode,
    Interval
};

constexpr std::uint32_t INTERVALS[] = {30, 60, 120, 180, 300, 600, 900, 1800, 3600};

lv_obj_t* content = nullptr;
lv_obj_t* gpsLabel = nullptr;
lv_obj_t* statusLabel = nullptr;
lv_obj_t* messageLabel = nullptr;
lv_obj_t* symbolDropdown = nullptr;
lv_obj_t* valueLabels[6] = {};

App::TrackerSettingsSaveHandler saveHandler = nullptr;
void* saveContext = nullptr;
bool draftEnabled = false;
bool draftTrailEnabled = false;
App::TrackerPositionSource draftSource = App::TrackerPositionSource::Gps;
App::TrackerPositionFormat draftFormat = App::TrackerPositionFormat::Uncompressed;
App::TrackerBeaconMode draftMode = App::TrackerBeaconMode::FixedInterval;
App::TrackerSymbol draftSymbol = App::TrackerSymbol::Car;
App::UiLanguage language = App::UiLanguage::Czech;
std::uint32_t draftInterval = 300;
std::uint32_t observedSettingsRevision = 0xFFFFFFFFU;

bool english() {
    return language == App::UiLanguage::English;
}

const char* text(const char* czech, const char* englishText) {
    return english() ? englishText : czech;
}

bool isSuccessMessage(const char* value) {
    return value != nullptr &&
        (std::strstr(value, "ulozeno") != nullptr ||
         std::strstr(value, "saved") != nullptr);
}

const char* sourceText(App::TrackerPositionSource source) {
    return source == App::TrackerPositionSource::Gps
        ? "GPS"
        : text("VYCHOZI", "DEFAULT");
}

const char* formatText(App::TrackerPositionFormat format) {
    if (format == App::TrackerPositionFormat::Compressed) {
        return text("KOMPRIMOVANA", "COMPRESSED");
    }
    return text("NORMALNI", "STANDARD");
}

const char* modeText(App::TrackerBeaconMode mode) {
    return mode == App::TrackerBeaconMode::SmartBeacon
        ? "SMARTBEACON"
        : text("PEVNY CAS", "FIXED TIME");
}

void refreshDraftLabels() {
    if (valueLabels[0] == nullptr) {
        return;
    }
    lv_label_set_text(valueLabels[0], draftEnabled ? text("ZAPNUT", "ON") : text("VYPNUT", "OFF"));
    lv_label_set_text(valueLabels[1], draftTrailEnabled ? text("ZAPNUT", "ON") : text("VYPNUT", "OFF"));
    lv_label_set_text(valueLabels[2], sourceText(draftSource));
    lv_label_set_text(valueLabels[3], formatText(draftFormat));
    lv_label_set_text(valueLabels[4], modeText(draftMode));
    char interval[32];
    std::snprintf(interval, sizeof(interval), "%u s", static_cast<unsigned>(draftInterval));
    lv_label_set_text(valueLabels[5], interval);
}

void copyFromSettings(const Services::SettingsService::ViewState& settings) {
    language = settings.uiLanguage;
    draftEnabled = settings.trackerEnabled;
    draftTrailEnabled = settings.trailEnabled;
    draftSource = settings.trackerSource;
    draftFormat = settings.trackerFormat;
    draftMode = settings.trackerMode;
    draftSymbol = settings.trackerSymbol;
    draftInterval = settings.trackerFixedIntervalSeconds;
    observedSettingsRevision = settings.revision;
    refreshDraftLabels();
    if (symbolDropdown != nullptr) {
        lv_dropdown_set_options(
            symbolDropdown,
            App::trackerSymbolDropdownOptions(language));
        lv_dropdown_set_selected(
            symbolDropdown,
            static_cast<std::uint16_t>(draftSymbol));
    }
}

void cycleInterval() {
    std::size_t index = 0;
    for (; index < sizeof(INTERVALS) / sizeof(INTERVALS[0]); ++index) {
        if (INTERVALS[index] >= draftInterval) {
            break;
        }
    }
    if (index >= sizeof(INTERVALS) / sizeof(INTERVALS[0]) ||
        INTERVALS[index] != draftInterval) {
        draftInterval = 300;
    } else {
        draftInterval = INTERVALS[(index + 1) % (sizeof(INTERVALS) / sizeof(INTERVALS[0]))];
    }
}

void fieldClicked(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }
    const Field field = static_cast<Field>(
        reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
    switch (field) {
        case Field::Enabled:
            draftEnabled = !draftEnabled;
            break;
        case Field::TrailEnabled:
            draftTrailEnabled = !draftTrailEnabled;
            break;
        case Field::Source:
            draftSource = draftSource == App::TrackerPositionSource::Gps
                ? App::TrackerPositionSource::DefaultPosition
                : App::TrackerPositionSource::Gps;
            if (draftSource == App::TrackerPositionSource::DefaultPosition &&
                draftMode == App::TrackerBeaconMode::SmartBeacon) {
                draftMode = App::TrackerBeaconMode::FixedInterval;
                setMessage(text(
                    "Defaultni poloha pouziva pevny interval.",
                    "The default position uses a fixed interval."));
            }
            break;
        case Field::Format:
            draftFormat = draftFormat == App::TrackerPositionFormat::Uncompressed
                ? App::TrackerPositionFormat::Compressed
                : App::TrackerPositionFormat::Uncompressed;
            break;
        case Field::Mode:
            if (draftMode == App::TrackerBeaconMode::FixedInterval) {
                if (draftSource != App::TrackerPositionSource::Gps) {
                    setMessage(text(
                        "SmartBeacon lze pouzit jen se zdrojem GPS.",
                        "SmartBeacon requires the GPS position source."));
                } else {
                    draftMode = App::TrackerBeaconMode::SmartBeacon;
                }
            } else {
                draftMode = App::TrackerBeaconMode::FixedInterval;
            }
            break;
        case Field::Interval:
            cycleInterval();
            break;
    }
    refreshDraftLabels();
}

void symbolChanged(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED || symbolDropdown == nullptr) {
        return;
    }
    const std::uint16_t selected = lv_dropdown_get_selected(symbolDropdown);
    if (selected < App::trackerSymbolCount()) {
        draftSymbol = static_cast<App::TrackerSymbol>(selected);
    }
}

void createSymbolRow() {
    lv_obj_t* row = lv_obj_create(content);
    lv_obj_set_size(row, 438, 48);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 9, 0);
    lv_obj_set_style_pad_left(row, 10, 0);
    lv_obj_set_style_pad_right(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* titleLabel = lv_label_create(row);
    lv_label_set_text(titleLabel, "APRS symbol");
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 0, 2);

    lv_obj_t* hintLabel = lv_label_create(row);
    lv_label_set_text(
        hintLabel,
        text("Symbol odesilane polohy", "Symbol used for transmitted position"));
    lv_obj_set_style_text_font(hintLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hintLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(hintLabel, LV_ALIGN_BOTTOM_LEFT, 0, -1);

    symbolDropdown = lv_dropdown_create(row);
    lv_obj_set_size(symbolDropdown, 208, 36);
    lv_obj_align(symbolDropdown, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(symbolDropdown, &lv_font_montserrat_14, 0);
    lv_dropdown_set_options(
        symbolDropdown,
        App::trackerSymbolDropdownOptions(language));
    lv_dropdown_set_selected(symbolDropdown, static_cast<std::uint16_t>(draftSymbol));
    lv_obj_add_event_cb(symbolDropdown, symbolChanged, LV_EVENT_VALUE_CHANGED, nullptr);
}

void saveClicked(lv_event_t* event) {
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        save();
    }
}

void createFieldRow(
    const char* title,
    const char* hint,
    Field field,
    std::size_t valueIndex) {

    lv_obj_t* row = lv_obj_create(content);
    lv_obj_set_size(row, 438, 42);
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
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 0, 3);

    lv_obj_t* hintLabel = lv_label_create(row);
    lv_label_set_text(hintLabel, hint);
    lv_obj_set_style_text_font(hintLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hintLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(hintLabel, LV_ALIGN_BOTTOM_LEFT, 0, -2);

    valueLabels[valueIndex] = lv_label_create(row);
    lv_obj_set_style_text_font(valueLabels[valueIndex], &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(valueLabels[valueIndex], lv_color_hex(0x56C7FF), 0);
    lv_obj_align(valueLabels[valueIndex], LV_ALIGN_RIGHT_MID, 0, 0);
}

}  // namespace

void create(
    const Services::SettingsService::ViewState& settings,
    const Services::GpsService::ViewState& gps,
    const Services::TrackerService::ViewState& tracker,
    App::TrackerSettingsSaveHandler handler,
    void* context) {

    saveHandler = handler;
    saveContext = context;
    language = settings.uiLanguage;
    symbolDropdown = nullptr;
    for (lv_obj_t*& valueLabel : valueLabels) {
        valueLabel = nullptr;
    }

    resetScreen();
    createHeader("APRS tracker");

    gpsLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(gpsLabel, 215);
    lv_label_set_long_mode(gpsLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(gpsLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(gpsLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(gpsLabel, LV_ALIGN_TOP_LEFT, 14, 52);

    statusLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(statusLabel, 235);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x42D392), 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_RIGHT, -14, 52);

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
        "Tracker",
        text("Zmena se projevi po ulozeni", "Changes are applied after saving"),
        Field::Enabled,
        0);
    createFieldRow(
        text("Stopar", "Trail logger"),
        text("Automaticky zaznam GPS trasy na SD", "Automatically record GPS track to SD"),
        Field::TrailEnabled,
        1);
    createFieldRow(
        text("Zdroj pozice", "Position source"),
        text("GPS nebo vychozi pozice", "GPS or default position"),
        Field::Source,
        2);
    createFieldRow(
        text("Format", "Format"),
        text("Normalni nebo Base-91", "Standard or Base-91 compressed"),
        Field::Format,
        3);
    createSymbolRow();
    createFieldRow(
        text("Planovani", "Scheduling"),
        text("Pevny interval / SmartBeacon", "Fixed interval / SmartBeacon"),
        Field::Mode,
        4);
    createFieldRow(
        "Interval",
        text("Pouziva se v rezimu pevny cas", "Used in fixed interval mode"),
        Field::Interval,
        5);

    lv_obj_t* saveButton = lv_btn_create(content);
    lv_obj_set_size(saveButton, 438, 40);
    lv_obj_set_style_bg_color(saveButton, lv_color_hex(0x2764D8), 0);
    lv_obj_add_event_cb(saveButton, saveClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* saveButtonLabel = lv_label_create(saveButton);
    lv_label_set_text(
        saveButtonLabel,
        text("Ulozit nastaveni trackeru", "Save tracker settings"));
    lv_obj_set_style_text_font(saveButtonLabel, &lv_font_montserrat_16, 0);
    lv_obj_center(saveButtonLabel);

    messageLabel = lv_label_create(content);
    lv_obj_set_width(messageLabel, 430);
    lv_label_set_long_mode(messageLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(
        messageLabel,
        text(
            "Stopare lze pozastavit na jeho samostatne strance.",
            "The trail logger can be paused on its own page."));
    lv_obj_set_style_text_font(messageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(messageLabel, lv_color_hex(0x92A7C7), 0);

    copyFromSettings(settings);
    update(gps, tracker, settings);
}

void update(
    const Services::GpsService::ViewState& gps,
    const Services::TrackerService::ViewState& tracker,
    const Services::SettingsService::ViewState& settings) {

    if (gpsLabel == nullptr || statusLabel == nullptr) {
        return;
    }

    if (settings.revision != observedSettingsRevision) {
        copyFromSettings(settings);
    }

    char gpsText[96];
    if (!gps.receiverDetected) {
        std::snprintf(
            gpsText,
            sizeof(gpsText),
            "%s",
            text("GPS: nenalezena", "GPS: not detected"));
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
            english() ? "GPS: fix, %.1f km/h, sat %u" : "GPS: fix, %.1f km/h, sat %u",
            static_cast<double>(gps.speedKmh),
            static_cast<unsigned>(gps.satellites));
    }
    lv_label_set_text(gpsLabel, gpsText);
    lv_label_set_text(statusLabel, tracker.statusText);
    lv_obj_set_style_text_color(
        statusLabel,
        tracker.active ? lv_color_hex(0x42D392) : lv_color_hex(0xFFB454),
        0);
}

void save() {
    if (saveHandler == nullptr) {
        return;
    }
    char error[128] = {};
    const bool result = saveHandler(
        draftEnabled,
        draftTrailEnabled,
        draftSource,
        draftFormat,
        draftMode,
        draftSymbol,
        draftInterval,
        error,
        sizeof(error),
        saveContext);
    setMessage(
        result
            ? text(
                "Nastaveni trackeru bylo ulozeno do NVS.",
                "Tracker settings were saved to NVS.")
            : error);
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

}  // namespace TrackerScreen
}  // namespace Ui
