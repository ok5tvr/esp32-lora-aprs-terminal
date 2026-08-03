#pragma once

#include <cstdint>

#include "app/app_types.h"

namespace App {

inline constexpr bool validAprsPath(AprsPath path) {
    return static_cast<std::uint8_t>(path) <=
        static_cast<std::uint8_t>(AprsPath::Wide2_2);
}

inline constexpr const char* aprsPathTnc2(AprsPath path) {
    switch (path) {
        case AprsPath::Wide1_1: return "WIDE1-1";
        case AprsPath::Wide2_2: return "WIDE2-2";
        case AprsPath::Direct:
        default: return "";
    }
}

inline constexpr const char* aprsPathLabel(AprsPath path) {
    switch (path) {
        case AprsPath::Wide1_1: return "WIDE1-1";
        case AprsPath::Wide2_2: return "WIDE2-2";
        case AprsPath::Direct:
        default: return "DIRECT";
    }
}

}  // namespace App
