#include "ui/ui_components.h"

#include <cstdint>

#include "board_pins.h"
#include "ui/ui_styles.h"

namespace Ui {
namespace {

NavigationHandler currentHandler = nullptr;
void* currentContext = nullptr;

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
    lv_obj_add_style(button, &Styles::navigationButton(), static_cast<lv_style_selector_t>(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_add_style(button, &Styles::navigationButtonPressed(), static_cast<lv_style_selector_t>(LV_PART_MAIN | LV_STATE_PRESSED));
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
