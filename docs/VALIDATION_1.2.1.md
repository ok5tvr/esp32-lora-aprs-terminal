# Validation checklist - version 1.2.1 GPS live NMEA sentence

## Host-side checks

- compile `GpsService` with strict `-Wall -Wextra -Werror` flags and test stubs
- feed a complete CR/LF-terminated GNRMC sentence
- verify the stored sentence starts with `$`, contains the checksum and excludes CR/LF
- feed a later LF-terminated GPGGA sentence
- verify the displayed state replaces the earlier sentence and updates the sentence type

## Device checks

1. Open **GPS diagnostika** with the GPS receiver disconnected.
   The final row must show `Cekam na prvni NMEA vetu...`.
2. Connect a 9600-baud NMEA receiver to UART2 RX GPIO17.
3. Verify that the final row changes to a sentence such as `$GNRMC,...*hh`.
4. Verify that the row refreshes as different GNRMC/GPGGA/GNGSA sentences arrive.
5. Verify that no CR/LF control characters appear on screen.
6. Verify that a long sentence is shortened with an ellipsis and does not overlap the navigation bar.
7. Keep LoRa reception and Tracker active and confirm that packet reception and beacon timing remain unchanged.

## Expected performance impact

The change copies at most 127 characters when a complete NMEA sentence arrives.
It performs no SD access, no dynamic allocation and no additional blocking I/O.
