# Validation record - firmware 2.7.9

## Scope

- dedicated APRS Beacon page
- persistent Beacon source/path/comment
- persistent Tracker path/comment
- path-bearing APRS position encoding
- Tracker, SmartBeacon and BOOT-beacon integration

## Host validation

1. APRS codec tests build and parse uncompressed and compressed frames.
2. A `WIDE1-1` frame is asserted exactly as `CALL>APRS,WIDE1-1:...`.
3. Invalid position data remains rejected.
4. Settings tests save and reload Tracker `WIDE2-2` plus comment and Beacon `WIDE1-1` plus comment.
5. Czech and English menu-order tests include the new Beacon item.
6. Tracker, Beacon, ScreenManager and AppController sources pass host syntax checks with project stubs.

## Required device validation

- build `waveshare-esp32-release` in PlatformIO
- upload by USB or Web OTA from an existing v2.7.8 A/B installation
- verify Beacon save persistence after reset
- verify GPS source rejects transmission without a fix
- verify default-position source transmits immediately
- inspect received TNC2 frames for `DIRECT`, `WIDE1-1` and `WIDE2-2`
- verify fixed Tracker, SmartBeacon and BOOT-button packets use the configured Tracker path and comment
- verify the central TX queue reports a manual-beacon item and returns to RX after transmission
