# Validation 1.3.1

## Host and static checks

- compile and run `test_aprs_symbol_lookup.cpp` with strict warnings;
- verify `/` selects the primary table and `\` selects the alternate table;
- verify `L&` selects alternate `&` and preserves the `L` overlay;
- verify compressed `a..j` overlays normalize to `0..9`;
- compile all three generated asset sources and `aprs_icons.cpp` against LVGL
  API stubs with `-Wall -Wextra -Werror`;
- run the descriptor test and verify both arrays contain exactly 94 RGB565 entries with 30 x 30 dimensions;
- verify the tracker generic selection is `//`, not `/.`;
- rerun TX queue, geography, DIGI core, station/message notification, GPS,
  power and Stopař host tests;
- confirm GPS GPIO4, LoRa DIO0 GPIO2 and all RA-02 SPI pins are unchanged.

## Device display test

1. Receive or inject primary symbols `/>`, `/#`, `/&`, `/[`, `/<`, `/-`, `/O`,
   `/^`, `/_`, `/b`, `/j`, `/k`, `/r`, `/s`, `/u` and `/v`.
2. Compare every displayed image with the upper table of the supplied chart.
3. Receive the same codes with alternate table `\` and verify the lower-table
   images are used. In particular compare `/j` versus `\j`, `/r` versus `\r`
   and `/Y` versus `\Y`.
4. Receive `L&`, `1#`, `A#` and another overlay symbol. Verify the alternate
   base image remains visible and the overlay character is centered above it.
5. Open both **Prijate stanice** and station detail and verify the same symbol
   is used on both screens.
6. Select **Obecny bod //** on the Tracker page, send a beacon and verify the
   red-dot symbol is transmitted and rendered instead of the red X.
7. Confirm LoRa RX/TX, navigation, GPS, Stopař and header service indicators
   continue to work.

## Required embedded build

```powershell
pio run -e waveshare-esp32-release
```
