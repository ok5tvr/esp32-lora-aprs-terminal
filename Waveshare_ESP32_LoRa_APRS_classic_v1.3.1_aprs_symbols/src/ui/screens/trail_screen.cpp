#include "ui/screens/trail_screen.h"

#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "ui/ui_components.h"

namespace Ui {
namespace TrailScreen {
namespace {

lv_obj_t* statusLabel = nullptr;
lv_obj_t* fileLabel = nullptr;
lv_obj_t* metricsLabel = nullptr;
lv_obj_t* pauseButton = nullptr;
lv_obj_t* pauseButtonLabel = nullptr;
lv_obj_t* logsList = nullptr;
lv_obj_t* logCountLabel = nullptr;
App::CommandHandler currentCommandHandler = nullptr;
void* currentCommandContext = nullptr;
std::uint32_t renderedRevision = 0xFFFFFFFFU;

lv_color_t stateColor(Services::TrailService::State state) {
    switch (state) {
        case Services::TrailService::State::Recording:
            return lv_color_hex(0x42D392);
        case Services::TrailService::State::AutoPaused:
        case Services::TrailService::State::ManualPaused:
        case Services::TrailService::State::WaitingForGps:
        case Services::TrailService::State::WaitingForSd:
            return lv_color_hex(0xFFB547);
        case Services::TrailService::State::Error:
            return lv_color_hex(0xF05B67);
        case Services::TrailService::State::Disabled:
        default:
            return lv_color_hex(0x92A7C7);
    }
}

void formatSize(std::uint32_t bytes, char* output, std::size_t capacity) {
    if (bytes >= 1024U * 1024U) {
        std::snprintf(output, capacity, "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    } else if (bytes >= 1024U) {
        std::snprintf(output, capacity, "%.1f kB", static_cast<double>(bytes) / 1024.0);
    } else {
        std::snprintf(output, capacity, "%u B", static_cast<unsigned>(bytes));
    }
}

void createLogRow(const Services::TrailService::LogEntry& log) {
    lv_obj_t* row = lv_obj_create(logsList);
    lv_obj_set_size(row, 438, 31);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 7, 0);
    lv_obj_set_style_pad_left(row, 9, 0);
    lv_obj_set_style_pad_right(row, 9, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name = lv_label_create(row);
    lv_obj_set_width(name, 315);
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    lv_label_set_text(name, log.name);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, 0);

    char sizeText[24] = {};
    formatSize(log.sizeBytes, sizeText, sizeof(sizeText));
    lv_obj_t* size = lv_label_create(row);
    lv_label_set_text(size, sizeText);
    lv_obj_set_style_text_font(size, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(size, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(size, LV_ALIGN_RIGHT_MID, 0, 0);
}

void pauseClicked(lv_event_t* event) {
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        togglePause();
    }
}

}  // namespace

void create(
    const Services::TrailService::ViewState& state,
    App::CommandHandler commandHandler,
    void* commandContext) {

    currentCommandHandler = commandHandler;
    currentCommandContext = commandContext;
    renderedRevision = 0xFFFFFFFFU;

    resetScreen();
    createHeader("Stopar");

    statusLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(statusLabel, 292);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_16, 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 14, 54);

    pauseButton = lv_btn_create(lv_scr_act());
    lv_obj_set_size(pauseButton, 148, 36);
    lv_obj_align(pauseButton, LV_ALIGN_TOP_RIGHT, -14, 49);
    lv_obj_set_style_bg_color(pauseButton, lv_color_hex(0x2764D8), 0);
    lv_obj_add_event_cb(pauseButton, pauseClicked, LV_EVENT_CLICKED, nullptr);
    pauseButtonLabel = lv_label_create(pauseButton);
    lv_obj_set_style_text_font(pauseButtonLabel, &lv_font_montserrat_14, 0);
    lv_obj_center(pauseButtonLabel);

    fileLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(fileLabel, 452);
    lv_label_set_long_mode(fileLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(fileLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(fileLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(fileLabel, LV_ALIGN_TOP_LEFT, 14, 86);

    metricsLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(metricsLabel, 452);
    lv_label_set_long_mode(metricsLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(metricsLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(metricsLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(metricsLabel, LV_ALIGN_TOP_LEFT, 14, 108);

    lv_obj_t* logsTitle = lv_label_create(lv_scr_act());
    lv_label_set_text(logsTitle, "Ulozene TXT logy");
    lv_obj_set_style_text_font(logsTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(logsTitle, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(logsTitle, LV_ALIGN_TOP_LEFT, 14, 133);

    logCountLabel = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(logCountLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(logCountLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(logCountLabel, LV_ALIGN_TOP_RIGHT, -14, 133);

    logsList = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(logsList);
    lv_obj_set_size(logsList, 452, 96);
    lv_obj_align(logsList, LV_ALIGN_TOP_MID, 0, 153);
    lv_obj_set_style_pad_left(logsList, 5, 0);
    lv_obj_set_style_pad_right(logsList, 5, 0);
    lv_obj_set_style_pad_top(logsList, 1, 0);
    lv_obj_set_style_pad_bottom(logsList, 1, 0);
    lv_obj_set_style_pad_row(logsList, 3, 0);
    lv_obj_set_flex_flow(logsList, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(logsList, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(logsList, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(logsList, LV_SCROLLBAR_MODE_AUTO);

    update(state);
}

void update(const Services::TrailService::ViewState& state) {
    if (statusLabel == nullptr || renderedRevision == state.revision) {
        return;
    }
    renderedRevision = state.revision;

    lv_label_set_text(statusLabel, state.statusText);
    lv_obj_set_style_text_color(statusLabel, stateColor(state.state), 0);

    char fileText[96] = {};
    std::snprintf(
        fileText,
        sizeof(fileText),
        "Soubor: %s",
        state.fileOpen && state.activeFile[0] != '\0' ? state.activeFile : "--");
    lv_label_set_text(fileLabel, fileText);

    const std::uint32_t hours = state.elapsedSeconds / 3600U;
    const std::uint32_t minutes = (state.elapsedSeconds / 60U) % 60U;
    const std::uint32_t seconds = state.elapsedSeconds % 60U;
    char metrics[128] = {};
    std::snprintf(
        metrics,
        sizeof(metrics),
        "Body %u | Trasa %.2f km | Cas %02u:%02u:%02u | Ztraceno %u",
        static_cast<unsigned>(state.pointsWritten),
        state.distanceKm,
        static_cast<unsigned>(hours),
        static_cast<unsigned>(minutes),
        static_cast<unsigned>(seconds),
        static_cast<unsigned>(state.droppedLines));
    lv_label_set_text(metricsLabel, metrics);

    const char* buttonText = "Pozastavit";
    if (!state.configuredEnabled) {
        buttonText = "Zapnout v Trackeru";
    } else if (state.manualPaused) {
        buttonText = "Pokracovat";
    }
    lv_label_set_text(pauseButtonLabel, buttonText);
    if (state.configuredEnabled) {
        lv_obj_clear_state(pauseButton, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(pauseButton, LV_STATE_DISABLED);
    }

    lv_obj_clean(logsList);
    char countText[16] = {};
    std::snprintf(countText, sizeof(countText), "%u/%u",
        static_cast<unsigned>(state.logCount),
        static_cast<unsigned>(Services::TrailService::MAX_LOGS));
    lv_label_set_text(logCountLabel, countText);

    if (state.logCount == 0U) {
        lv_obj_t* empty = lv_label_create(logsList);
        lv_label_set_text(empty, state.sdMounted ? "Na SD karte zatim nejsou zadne logy." : "SD karta neni dostupna.");
        lv_obj_set_width(empty, 420);
        lv_label_set_long_mode(empty, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(empty, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(empty, lv_color_hex(0x92A7C7), 0);
    } else {
        for (std::uint8_t index = 0; index < state.logCount; ++index) {
            createLogRow(state.logs[index]);
        }
    }
}

void togglePause() {
    if (currentCommandHandler != nullptr) {
        currentCommandHandler(App::Command::ToggleTrailPause, currentCommandContext);
    }
}

void scroll(int direction) {
    if (logsList == nullptr || direction == 0) {
        return;
    }
    lv_obj_scroll_by(logsList, 0, direction > 0 ? -34 : 34, LV_ANIM_ON);
}

void setMessage(const char* text) {
    if (statusLabel == nullptr) {
        return;
    }
    lv_label_set_text(statusLabel, text != nullptr ? text : "");
    lv_obj_set_style_text_color(
        statusLabel,
        text != nullptr && std::strstr(text, "spusten") != nullptr
            ? lv_color_hex(0x42D392)
            : lv_color_hex(0xFFB547),
        0);
}

}  // namespace TrailScreen
}  // namespace Ui
