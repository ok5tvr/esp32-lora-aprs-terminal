#pragma once

#include <lvgl.h>

namespace Ui::AprsIcons {

// Creates a compact 32 x 32 APRS symbol widget. The table identifier may be
// '/', '\\', or an APRS overlay character. Unknown symbols retain their
// original two-character code as a fallback.
lv_obj_t* create(
    lv_obj_t* parent,
    char symbolTable,
    char symbolCode,
    bool symbolAvailable = true);

}  // namespace Ui::AprsIcons
