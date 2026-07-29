#pragma once

#include <lvgl.h>

namespace Ui {
namespace Styles {

void begin();
lv_style_t& menuRowNormal();
lv_style_t& menuRowSelected();
lv_style_t& navigationButton();
lv_style_t& navigationButtonPressed();

}  // namespace Styles
}  // namespace Ui
