#pragma once

#include <cstddef>
#include <lvgl.h>

namespace Ui::AprsIconAssets {

inline constexpr std::size_t SYMBOL_COUNT =
    static_cast<std::size_t>('~' - '!' + 1);

// Full-color APRS symbol tables generated from the reference chart supplied
// for this firmware. Index 0 is symbol code '!', index 93 is '~'.
extern const lv_img_dsc_t primary[SYMBOL_COUNT];
extern const lv_img_dsc_t alternate[SYMBOL_COUNT];
extern const lv_img_dsc_t unknown;

// Alpha-only variants retained for recolored status indicators in the header.
extern const lv_img_dsc_t car;
extern const lv_img_dsc_t digipeater;
extern const lv_img_dsc_t gateway;

}  // namespace Ui::AprsIconAssets
