# Release History

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
