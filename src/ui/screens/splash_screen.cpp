#include "ui/screens/splash_screen.h"

#include <Arduino.h>
#include <lvgl.h>

#include "app_config.h"
#include "app/localization.h"
#include "ui/ui_components.h"

namespace Ui {
namespace SplashScreen {
namespace {
std::uint32_t startedAt = 0;
}

void create() {
    resetScreen();

    lv_obj_t* title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, AppConfig::FIRMWARE_NAME);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -45);

    lv_obj_t* subtitle = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(subtitle, "ESP32 classic | RA-02 | SD | v%s", AppConfig::FIRMWARE_VERSION);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, -5);

    lv_obj_t* author = lv_label_create(lv_scr_act());
    lv_label_set_text(
        author,
        App::Localization::text("Vytvoril: OK5TVR", "Created by: OK5TVR"));
    lv_obj_set_style_text_font(author, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(author, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(author, LV_ALIGN_CENTER, 0, 28);

    lv_obj_t* progress = lv_bar_create(lv_scr_act());
    lv_obj_set_size(progress, 300, 12);
    lv_obj_align(progress, LV_ALIGN_CENTER, 0, 62);
    lv_obj_set_style_bg_color(progress, lv_color_hex(0x24344F), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress, lv_color_hex(0x2764D8), LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(progress, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_bar_set_range(progress, 0, 100);
    lv_bar_set_value(progress, 0, LV_ANIM_OFF);
    lv_bar_set_value(progress, 100, LV_ANIM_ON);

    startedAt = millis();
}

bool finished(std::uint32_t now) {
    return now - startedAt >= AppConfig::SPLASH_DURATION_MS;
}

}  // namespace SplashScreen
}  // namespace Ui
