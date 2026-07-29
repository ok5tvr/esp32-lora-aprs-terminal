# APRS icon renderer

Version 0.9.1 replaces the raw two-character symbol text in the station and
weather lists with a compact LVGL image widget.

## Included base icons

The first small icon set contains:

- generic station / map point
- car and common vehicle-family codes
- person
- fixed QTH / house
- weather station
- digipeater
- gateway / iGate
- balloon
- aircraft
- boat
- bicycle
- repeater / radio site
- unknown-symbol fallback

The images are 24 x 24 alpha-only bitmaps linked into firmware Flash. LVGL
recolors them at runtime, so only one copy of each shape is required.

## LoRa iGate

The LoRa APRS iGate convention is the alternate gateway symbol `&` with the
`L` overlay. On air the overlay replaces the table-identifier character, so
the stored symbol pair is:

```text
L&
```

The renderer selects the gateway diamond and draws the letter `L` over it.
The same overlay mechanism also works for other digit or letter overlays.

## Unsupported symbols

When no graphical icon is available, the unknown icon is shown together with
the original two-character APRS symbol code. No protocol information is lost.

## Files

```text
src/ui/aprs_icons.h
src/ui/aprs_icons.cpp
src/ui/icons/aprs_icon_assets.h
src/ui/icons/aprs_icon_assets.cpp
```
