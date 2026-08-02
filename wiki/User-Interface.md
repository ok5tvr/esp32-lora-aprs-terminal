# User Interface

## Main menu

Firmware v2.7.7 uses this menu order:

1. LoRa APRS
2. Received stations
3. Messages
4. Weather stations
5. Map
6. Tracker
7. Trail logger
8. DIGI / iGate
9. GPS receiver
10. Astronomy
11. Diagnostics
12. Power
13. Settings

## Navigation controls

Each screen uses the touchscreen and a common bottom control row:

- **Up**: previous item or scroll upward
- **Down**: next item or scroll downward
- **OK**: open, confirm, select or recenter depending on screen
- **Back**: return to the previous screen or main menu

Scrollable screens may also be dragged directly by touch.

## Header indicators

### GPS

- grey: no recent UART traffic
- dark orange: traffic but no complete current NMEA state
- orange: receiver detected, no current fix
- green: current GPS fix

### APRS messages

An orange bell and red badge indicate new directed, ALL, QST or CQ messages. Opening **Messages** marks them as seen.

### Newly heard entity

A blue RF indicator and badge show newly inserted APRS stations, objects or items. Opening **Received stations** marks them as seen.

### Tracker

- grey: periodic tracker disabled
- amber: enabled but waiting for a usable position
- green: active scheduling with a valid position

### Trail logger

- grey: disabled
- amber: waiting, paused or not ready
- green: recording
- red: SD or file error

### Digipeater

- grey: disabled
- purple: enabled

### LoRa iGate

- grey: disabled
- amber: Wi-Fi/APRS-IS not verified
- green: verified RF-to-IS operation

### Clock

- `--:--`: no valid RTC or GPS time
- white: RTC or holdover time
- green: current GPS time is the active reference

The RTC stores UTC. The interface displays CET/CEST using European daylight-saving rules.

## Power summary

The right side of normal screen headers shows battery percentage, battery voltage and a status symbol:

- red battery: critical battery
- green lightning: charging
- blue USB: USB connected without active charging
- white battery: battery operation

## Language

Open **Settings > Interface language** and choose Czech or English. The active screen is rebuilt immediately after saving; a restart is not required.

Protocol data such as callsigns, APRS frames, NMEA sentences and units are intentionally not translated.
