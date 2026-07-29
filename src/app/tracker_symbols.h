#pragma once

#include <cstddef>
#include <cstdint>

#include "app/app_types.h"

namespace App {

struct TrackerSymbolDefinition {
    TrackerSymbol id;
    const char* label;
    char table;
    char code;
};

inline constexpr TrackerSymbolDefinition TRACKER_SYMBOL_DEFINITIONS[] = {
    {TrackerSymbol::Car,        "Auto",          '/', '>'},
    {TrackerSymbol::Person,     "Chodec",        '/', '['},
    {TrackerSymbol::Bicycle,    "Jizdni kolo",   '/', 'b'},
    {TrackerSymbol::Motorcycle, "Motocykl",      '/', '<'},
    {TrackerSymbol::House,      "QTH / dum",     '/', '-'},
    {TrackerSymbol::Boat,       "Lod",           '/', 's'},
    {TrackerSymbol::Aircraft,   "Letadlo",       '/', '^'},
    {TrackerSymbol::Balloon,    "Balon",         '/', 'O'},
    {TrackerSymbol::Weather,    "Meteostanice",  '/', '_'},
    {TrackerSymbol::Generic,    "Obecna stanice",'/', '.'},
    {TrackerSymbol::LoraIgate,  "LoRa iGate",    'L', '&'},
};


inline constexpr char TRACKER_SYMBOL_DROPDOWN_OPTIONS[] =
    "Auto />\n"
    "Chodec /[\n"
    "Jizdni kolo /b\n"
    "Motocykl /<\n"
    "QTH / dum /-\n"
    "Lod /s\n"
    "Letadlo /^\n"
    "Balon /O\n"
    "Meteostanice /_\n"
    "Obecna stanice /.\n"
    "LoRa iGate L&";

static_assert(
    sizeof(TRACKER_SYMBOL_DEFINITIONS) / sizeof(TRACKER_SYMBOL_DEFINITIONS[0]) ==
        static_cast<std::size_t>(TrackerSymbol::Count),
    "Tracker symbol table and enum must stay in sync");

inline constexpr std::size_t trackerSymbolCount() {
    return sizeof(TRACKER_SYMBOL_DEFINITIONS) /
        sizeof(TRACKER_SYMBOL_DEFINITIONS[0]);
}

inline constexpr bool validTrackerSymbol(TrackerSymbol symbol) {
    return static_cast<std::uint8_t>(symbol) <
        static_cast<std::uint8_t>(TrackerSymbol::Count);
}

inline constexpr const TrackerSymbolDefinition& trackerSymbolDefinition(
    TrackerSymbol symbol) {

    const std::size_t index = validTrackerSymbol(symbol)
        ? static_cast<std::size_t>(symbol)
        : static_cast<std::size_t>(TrackerSymbol::Car);
    return TRACKER_SYMBOL_DEFINITIONS[index];
}

}  // namespace App
