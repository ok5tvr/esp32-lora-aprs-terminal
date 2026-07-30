# Validation 1.2.4

## Purpose

Verify that the terminal receives NMEA data from the external GPS receiver at
4800 baud without changing the existing GPS parser, tracker or diagnostics
behaviour.

## Procedure

1. Build firmware version 1.2.4 for `waveshare-esp32-release`.
2. Connect a 3.3 V logic-level NMEA receiver configured for 4800 baud, 8-N-1 to
   UART2 RX GPIO17.
3. Open the serial monitor at 115200 baud and confirm the startup message reports
   `UART2 listening at 4800 baud on GPIO17`.
4. Open **GPS diagnostika** and confirm the UART row reports `4800 Bd`.
5. Confirm that complete NMEA sentences are shown and update continuously.
6. Obtain a valid fix and verify that position, satellites, HDOP, altitude,
   speed and course update normally.
7. Enable the APRS tracker and Stopař and verify that both use the GPS fix.
8. Confirm that LoRa reception remains active while GPS data are arriving.

## Acceptance criteria

- GPS UART starts at 4800 baud.
- Valid 4800-baud NMEA data are parsed without checksum regressions.
- GPS diagnostics show 4800 Bd and the latest complete sentence.
- Tracker and Stopař receive valid fixes.
- No new blocking operations or SD writes are introduced by this change.

## Important limitation

The firmware only changes the ESP32 UART receive speed. A GPS receiver still
configured for 9600 baud will not be detected. Reprogramming the receiver itself
requires a module-specific command or configuration utility.
