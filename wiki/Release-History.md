# Release History

## v2.7.11

- SmartBeacon profiles for car, bicycle and walking
- profile-specific speed intervals, turn sensitivity and movement confirmation
- start-moving and stopped beacons
- Tracker timing based on completed RF TX instead of queue insertion
- exact TX sequence matching and retry after failure/timeout
- APRS `=` position identifier for a messaging-capable terminal
- 40-character limit for compressed-position comments
- last completed beacon reason on the Tracker page

## v2.7.10

- Arduino-ESP32 `Update.write(uint8_t*, size_t)` compatibility fix

## v2.7.9

- dedicated one-shot APRS Beacon page
- independent GPS/default source, path and comment stored in NVS
- configurable Tracker path and comment
- `DIRECT`, `WIDE1-1` and `WIDE2-2` position-frame encoding
- Tracker, SmartBeacon and BOOT packets share the saved Tracker path/comment

## v2.7.8

- live internal heap, largest-block, minimum-heap, PSRAM and loop-stack diagnostics
- uptime and last ESP32 reset reason
- D/1-9+/I/? route badges in the Received Stations list
- explicit APRS-IS/Internet path classification
- hardened Web OTA with progress, status, safe stop and ESP32 application validation
- corrected two-slot 7 MB A/B OTA partition table
- OTA AP preserved during iGate reconnects

## v2.7.7

- fixed startup stack overflow in `StationStore::clear()`
- retained complete APRS route analysis

## v2.7.6

- complete APRS path storage
- DIRECT / VIA DIGI detection
- hop count and last used digipeater
- separate direct and repeated reception counters
- age of last direct reception
- scrollable station detail

## v2.7.5

- browser-based Web OTA
- local WPA2 maintenance AP at `192.168.4.1`
- two 7 MB OTA application slots

## v2.7.4

- persistent variable-interval battery history
- ±1% confirmed battery changes
- mode-transition and one-hour checkpoints
- CRC32-protected NVS storage

## v2.7.3

- battery history graph
- actual LoRa frequency in Diagnostics
- diagnostics history reset after RF profile change

## v2.7.2

- offline astronomy page
- Sun altitude and Moon phase visualization

## v2.7.0 and earlier highlights

- Czech/English interface
- configurable LoRa profile
- RTC/GPS local clock
- battery display power policy
- touch-pannable offline map
- central TX queue and radio recovery
- complete APRS symbol tables
- GPS tracker and Trail logger
- WIDE digipeater and receive-only APRS-IS iGate
- APRS messaging and extended parser

For the complete detailed history, see the repository `CHANGELOG.md`.
