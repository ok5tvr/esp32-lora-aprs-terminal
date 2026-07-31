#include "ui/screens/placeholder_screen.h"

#include <lvgl.h>

#include "app/localization.h"
#include "ui/ui_components.h"

namespace Ui {
namespace PlaceholderScreen {
namespace {
lv_obj_t* messageLabel = nullptr;
}

void create(const char* title, const char* description) {
    resetScreen();
    createHeader(title);

    lv_obj_t* card = lv_obj_create(lv_scr_act());
    lv_obj_set_size(card, 444, 180);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 62);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_pad_all(card, 18, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* descriptionLabel = lv_label_create(card);
    lv_obj_set_width(descriptionLabel, 400);
    lv_label_set_long_mode(descriptionLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(descriptionLabel, description);
    lv_obj_set_style_text_font(descriptionLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(descriptionLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(descriptionLabel, LV_ALIGN_TOP_LEFT, 0, 10);

    lv_obj_t* chip = lv_label_create(card);
    lv_label_set_text(
        chip,
        App::Localization::text(
            "Modul bude doplnen samostatnym service a obrazovkou.",
            "The module will be added as a separate service and screen."));
    lv_obj_set_style_text_font(chip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(chip, lv_color_hex(0x42D392), 0);
    lv_obj_align(chip, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    messageLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(
        messageLabel,
        App::Localization::text(
            "Sipka vlevo = hlavni menu",
            "Left arrow = main menu"));
    lv_obj_set_style_text_font(messageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(messageLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(messageLabel, LV_ALIGN_BOTTOM_MID, 0, -72);
}

void setMessage(const char* text) {
    if (messageLabel != nullptr) {
        lv_label_set_text(messageLabel, text);
    }
}

}  // namespace PlaceholderScreen
}  // namespace Ui
