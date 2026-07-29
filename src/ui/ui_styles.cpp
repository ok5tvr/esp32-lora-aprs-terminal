#include "ui/ui_styles.h"

namespace Ui {
namespace Styles {
namespace {

lv_style_t rowNormal;
lv_style_t rowSelected;
lv_style_t navButton;
lv_style_t navButtonPressed;
bool initialized = false;

}  // namespace

void begin() {
    if (initialized) {
        return;
    }

    lv_style_init(&rowNormal);
    lv_style_set_bg_color(&rowNormal, lv_color_hex(0x17243A));
    lv_style_set_bg_opa(&rowNormal, LV_OPA_COVER);
    lv_style_set_border_color(&rowNormal, lv_color_hex(0x31425F));
    lv_style_set_border_width(&rowNormal, 1);
    lv_style_set_radius(&rowNormal, 10);
    lv_style_set_text_color(&rowNormal, lv_color_hex(0xEAF1FF));
    lv_style_set_pad_left(&rowNormal, 14);
    lv_style_set_pad_right(&rowNormal, 14);

    lv_style_init(&rowSelected);
    lv_style_set_bg_color(&rowSelected, lv_color_hex(0x2764D8));
    lv_style_set_bg_opa(&rowSelected, LV_OPA_COVER);
    lv_style_set_border_color(&rowSelected, lv_color_hex(0x6EA0FF));
    lv_style_set_border_width(&rowSelected, 2);
    lv_style_set_radius(&rowSelected, 10);
    lv_style_set_text_color(&rowSelected, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_left(&rowSelected, 14);
    lv_style_set_pad_right(&rowSelected, 14);
    lv_style_set_shadow_width(&rowSelected, 12);
    lv_style_set_shadow_opa(&rowSelected, LV_OPA_30);
    lv_style_set_shadow_color(&rowSelected, lv_color_hex(0x2764D8));

    lv_style_init(&navButton);
    lv_style_set_bg_color(&navButton, lv_color_hex(0x24344F));
    lv_style_set_bg_opa(&navButton, LV_OPA_COVER);
    lv_style_set_border_color(&navButton, lv_color_hex(0x405778));
    lv_style_set_border_width(&navButton, 1);
    lv_style_set_radius(&navButton, 12);
    lv_style_set_text_color(&navButton, lv_color_hex(0xFFFFFF));

    lv_style_init(&navButtonPressed);
    lv_style_set_bg_color(&navButtonPressed, lv_color_hex(0x2764D8));
    lv_style_set_transform_width(&navButtonPressed, -2);
    lv_style_set_transform_height(&navButtonPressed, -2);

    initialized = true;
}

lv_style_t& menuRowNormal() { return rowNormal; }
lv_style_t& menuRowSelected() { return rowSelected; }
lv_style_t& navigationButton() { return navButton; }
lv_style_t& navigationButtonPressed() { return navButtonPressed; }

}  // namespace Styles
}  // namespace Ui
