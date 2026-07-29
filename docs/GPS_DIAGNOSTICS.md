# GPS diagnostics and Maidenhead locator

Firmware 0.9.2 extends the GPS page with live NMEA diagnostics.

## Detection states

The service distinguishes four levels:

1. UART started but no bytes received.
2. Serial traffic received but no complete NMEA sentence ending with LF.
3. A complete NMEA sentence received, but no recent sentence passed checksum validation.
4. Valid NMEA packets received, with or without a current position fix.

The default detection timeout is 10 seconds. A position fix is considered current for
5 seconds. Both values are defined in `include/app_config.h`.

## Displayed values

- UART number, RX GPIO and baud rate
- last NMEA sentence type, for example `GNRMC` or `GPGGA`
- age of the latest complete and latest checksum-valid NMEA sentence
- latitude, longitude and altitude
- six-character Maidenhead locator
- speed, course and 16-point cardinal direction
- satellites, HDOP, UTC time and date
- sentence count, characters per second and checksum counters

## Main menu

The main header shows the locator of the active reference position:

- `GPS JN69PS` when a current GPS fix is available
- `DEF JN69PS` when the configured default position is used

The locator is recalculated whenever the reference position changes.
