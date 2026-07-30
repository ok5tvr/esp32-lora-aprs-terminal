# APRS symbol renderer

Version 1.3.1 replaces the former small category-based icon set with complete
primary and alternate APRS symbol tables generated from the supplied reference
chart.

## Table selection

The renderer now resolves both APRS symbol characters:

- `/` selects the primary table shown in the upper half of the chart;
- `\` selects the alternate table shown in the lower half;
- a digit or letter in the table position selects the alternate-table base
  symbol and draws that character as an overlay;
- compressed table identifiers `a` through `j` are normalized to numeric
  overlays `0` through `9`.

The symbol code range `!` through `~` contains 94 entries in each table. The
code is converted directly to an array index, so symbols that share the same
second character but use a different table no longer collapse to the same
image. For example, `/j` is the primary jeep while `\j` is the alternate
excavator.

## LoRa iGate

The LoRa APRS iGate convention remains `L&`. The `L` is the overlay/table
character, so the base image is alternate `\&` (black diamond) and the letter
`L` is rendered above it.

## Tracker correction

The former generic tracker selection used `/.`, which is the primary red-X
symbol in the reference table. Version 1.3.1 changes this selection to `//`,
the primary red-dot symbol, and labels it **Obecny bod**.

## Image format

Each table icon is stored as a 30 x 30 RGB565 LVGL image. The station and
weather lists place it in a 32 x 32 white rounded container. The complete two
symbol tables occupy approximately 340 kB of firmware Flash and do not consume
working RAM beyond the normal LVGL object state.

The small tracker, DIGI and iGate header indicators remain alpha-only images so
they can still be recolored according to service state.

## Files

```text
src/ui/aprs_symbol_lookup.h
src/ui/aprs_icons.h
src/ui/aprs_icons.cpp
src/ui/icons/aprs_icon_assets.h
src/ui/icons/aprs_icon_assets.cpp
src/ui/icons/aprs_icon_assets_primary.cpp
src/ui/icons/aprs_icon_assets_alternate.cpp
tools/generate_aprs_symbol_assets.py
```

The generator expects a two-table chart with symbols `!` through `~` arranged
in rows of 16. Regenerate the assets only when the source chart or icon size is
changed.
