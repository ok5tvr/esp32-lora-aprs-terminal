#include "ui/ui_components.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "board_pins.h"
#include "ui/ui_styles.h"

namespace Ui {
namespace {

NavigationHandler currentHandler = nullptr;
void* currentContext = nullptr;
lv_obj_t* headerPowerLabel = nullptr;

void formatVoltageCz(char* output, std::size_t capacity, std::uint16_t millivolts) {
    if (output == nullptr || capacity == 0U) {
        return;
    }
    std::snprintf(output, capacity, "%.2f", static_cast<double>(millivolts) / 1000.0);
    if (char* decimal = std::strchr(output, '.')) {
        *decimal = ',';
    }
}

void navigationEvent(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || currentHandler == nullptr) {
        return;
    }
    const std::intptr_t raw = reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event));
    currentHandler(static_cast<App::NavigationAction>(raw), currentContext);
}

void createButton(lv_obj_t* parent, const char* text, App::NavigationAction action) {
    lv_obj_t* button = lv_btn_create(parent);
    lv_obj_set_size(button, 110, 52);
    const auto defaultSelector = static_cast<lv_style_selector_t>(
        static_cast<std::uint32_t>(LV_PART_MAIN) | static_cast<std::uint32_t>(LV_STATE_DEFAULT));
    const auto pressedSelector = static_cast<lv_style_selector_t>(
        static_cast<std::uint32_t>(LV_PART_MAIN) | static_cast<std::uint32_t>(LV_STATE_PRESSED));
    lv_obj_add_style(button, &Styles::navigationButton(), defaultSelector);
    lv_obj_add_style(button, &Styles::navigationButtonPressed(), pressedSelector);
    lv_obj_add_event_cb(
        button,
        navigationEvent,
        LV_EVENT_CLICKED,
        reinterpret_cast<void*>(static_cast<std::intptr_t>(action)));

    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_22, 0);
    lv_obj_center(label);
}

}  // namespace

void resetScreen() {
    lv_obj_t* screen = lv_scr_act();
    lv_obj_clean(screen);
    headerPowerLabel = nullptr;
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B1424), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
}

void createHeader(const char* title) {
    lv_obj_t* header = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, BoardPins::SCREEN_WIDTH, 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x101D31), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0x263A59), 0);
    lv_obj_set_style_border_width(header, 1, 0);

    lv_obj_t* label = lv_label_create(header);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 14, 0);

    headerPowerLabel = lv_label_create(header);
    lv_obj_set_width(headerPowerLabel, 148);
    lv_label_set_long_mode(headerPowerLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(headerPowerLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(headerPowerLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(headerPowerLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(headerPowerLabel, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_label_set_text(headerPowerLabel, "PWR --");
}

void updateHeaderPower(const Services::PowerService::ViewState& state) {
    if (headerPowerLabel == nullptr) {
        return;
    }

    if (!state.available) {
        lv_label_set_text(headerPowerLabel, "PWR --");
        lv_obj_set_style_text_color(headerPowerLabel, lv_color_hex(0x46566E), 0);
        return;
    }

    const char* symbol = LV_SYMBOL_BATTERY_EMPTY;
    if (state.charging) {
        symbol = LV_SYMBOL_CHARGE;
    } else if (state.vbusConnected) {
        symbol = LV_SYMBOL_USB;
    } else if (state.batteryPercent >= 80U) {
        symbol = LV_SYMBOL_BATTERY_FULL;
    } else if (state.batteryPercent >= 55U) {
        symbol = LV_SYMBOL_BATTERY_3;
    } else if (state.batteryPercent >= 30U) {
        symbol = LV_SYMBOL_BATTERY_2;
    } else if (state.batteryPercent >= 12U) {
        symbol = LV_SYMBOL_BATTERY_1;
    }

    char text[64];
    char voltage[12];
    if (state.batteryConnected) {
        formatVoltageCz(voltage, sizeof(voltage), state.batteryVoltageMv);
        if (state.batteryPercentValid) {
            std::snprintf(
                text,
                sizeof(text),
                "%u%%  %sV  %s",
                static_cast<unsigned>(state.batteryPercent),
                voltage,
                symbol);
        } else {
            std::snprintf(text, sizeof(text), "--%%  %sV  %s", voltage, symbol);
        }
    } else if (state.vbusConnected) {
        formatVoltageCz(voltage, sizeof(voltage), state.vbusVoltageMv);
        std::snprintf(text, sizeof(text), "USB  %sV  %s", voltage, symbol);
    } else {
        std::snprintf(text, sizeof(text), "BAT --  %s", symbol);
    }
    lv_label_set_text(headerPowerLabel, text);

    lv_color_t color = lv_color_hex(0xF4F7FF);
    if (state.criticalBattery) {
        color = lv_color_hex(0xF05B67);
    } else if (state.charging) {
        color = lv_color_hex(0x42D392);
    } else if (state.vbusConnected) {
        color = lv_color_hex(0x56C7FF);
    }
    lv_obj_set_style_text_color(headerPowerLabel, color, 0);
}

void createNavigationBar(NavigationHandler handler, void* context) {
    currentHandler = handler;
    currentContext = context;

    lv_obj_t* bar = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, BoardPins::SCREEN_WIDTH, 68);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x101D31), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(bar, 8, 0);
    lv_obj_set_style_pad_right(bar, 8, 0);
    lv_obj_set_style_pad_top(bar, 8, 0);
    lv_obj_set_style_pad_bottom(bar, 8, 0);
    lv_obj_set_style_pad_column(bar, 6, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        bar,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    createButton(bar, LV_SYMBOL_UP, App::NavigationAction::Up);
    createButton(bar, LV_SYMBOL_DOWN, App::NavigationAction::Down);
    createButton(bar, LV_SYMBOL_OK " OK", App::NavigationAction::Confirm);
    createButton(bar, LV_SYMBOL_LEFT, App::NavigationAction::Back);
}

}  // namespace Ui
