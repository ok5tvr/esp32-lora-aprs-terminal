# Settings Reference

## General settings

### Callsign

Enter a valid amateur-radio callsign with optional SSID. Example:

```text
OK5TVR-15
```

Avoid explicit `-0` for APRS-IS use.

### Default position

Latitude and longitude are used when no current GPS fix is available. They support map centering, station navigation, astronomy and tracker operation when **Default** is selected as the tracker source.

### Interface language

- Czech
- English

The language changes immediately after saving.

## Display settings

### Battery brightness

Range:

```text
10% to 100%
```

### Turn display off

Options:

```text
Never, 30 s, 60 s, 2 min, 5 min
```

On battery, the display first dims to 15% after 30 seconds. USB operation forces full brightness.

## LoRa profile

### CZE APRS preset

```text
Frequency: 433.775 MHz
Bandwidth: 125 kHz
Spreading factor: SF12
Coding rate: 4/5
TX power: 10 dBm
```

### Custom profile

Supported ranges include:

- frequency: 410.000 to 525.000 MHz
- bandwidth: 62.5, 125, 250 or 500 kHz
- spreading factor: SF7 through SF12
- coding rate: 4/5 through 4/8
- TX power: supported selectable levels up to 17 dBm

The LoRa sync word, explicit header, preamble and compatibility CRC behavior remain fixed.

A profile change waits until active transmission and the central TX queue are clear. Only the RA-02 radio is reinitialized; GPS, display and other services continue.

Changing the RF profile clears the old background-RSSI diagnostics history so measurements from different frequencies are not mixed.

## OTA update

Enable **Start web OTA mode**, save, then connect to the maintenance access point. OTA disables itself after reboot. The first v2.7.8 installation must be completed by USB so the two-slot A/B partition table is present.

## DIGI / iGate settings

These settings are on a separate page:

- digipeater enable
- DIGI mode
- maximum WIDE request
- RX iGate enable
- Wi-Fi credentials
- APRS-IS server, port and passcode
- optional filter

See [DIGI and iGate](DIGI-and-iGate).

## Persistence

Settings are stored in ESP32 NVS and survive reset and power loss. If NVS is unavailable, the interface reports the problem and falls back to compiled defaults where possible.

## Tracker path and comment

The Tracker page stores a separate APRS path and comment in NVS. Path options are:

- `DIRECT`: no digipeater path is inserted
- `WIDE1-1`: one local fill-in request
- `WIDE2-2`: two-hop regional request

The comment accepts 1 to 48 printable characters. Standard position packets use the complete comment; compressed APRS packets safely transmit the first 40 characters. These fields apply to fixed-interval Tracker packets, SmartBeacon packets and manual BOOT-button beacons.

SmartBeacon also stores a profile: **Car**, **Bicycle** or **Walking**. The selected profile controls adaptive speed intervals, turn sensitivity and the confirmation times for start-moving and stopped beacons.

## Dedicated Beacon settings

The Beacon page stores its own position source, APRS path and comment. These values do not overwrite the Tracker source, path or comment. Beacon format and APRS symbol are intentionally inherited from Tracker settings.
