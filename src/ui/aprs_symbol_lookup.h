#pragma once

#include <cstddef>
#include <cstdint>

namespace Ui::AprsSymbols {

inline constexpr char FIRST_CODE = '!';
inline constexpr char LAST_CODE = '~';
inline constexpr std::size_t SYMBOL_COUNT =
    static_cast<std::size_t>(LAST_CODE - FIRST_CODE + 1);

struct Lookup {
    bool valid = false;
    bool alternate = false;
    std::uint8_t index = 0;
    char overlay = '\0';
};

inline constexpr bool isOverlayTable(char table) {
    return (table >= '0' && table <= '9') ||
           (table >= 'A' && table <= 'Z') ||
           (table >= 'a' && table <= 'z');
}

inline constexpr char normalizeOverlay(char table) {
    // In compressed APRS positions a..j represent numeric overlays 0..9.
    if (table >= 'a' && table <= 'j') {
        return static_cast<char>('0' + (table - 'a'));
    }
    return table;
}

inline constexpr Lookup resolve(
    char symbolTable,
    char symbolCode,
    bool symbolAvailable = true) {

    Lookup result;
    if (!symbolAvailable || symbolCode < FIRST_CODE || symbolCode > LAST_CODE) {
        return result;
    }

    result.index = static_cast<std::uint8_t>(symbolCode - FIRST_CODE);

    if (symbolTable == '/') {
        result.valid = true;
        result.alternate = false;
        return result;
    }

    if (symbolTable == '\\') {
        result.valid = true;
        result.alternate = true;
        return result;
    }

    if (isOverlayTable(symbolTable)) {
        result.valid = true;
        result.alternate = true;
        result.overlay = normalizeOverlay(symbolTable);
        return result;
    }

    return result;
}

}  // namespace Ui::AprsSymbols
