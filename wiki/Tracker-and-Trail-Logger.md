# Tracker and Trail Logger

The APRS tracker and the Trail logger are independent services.

- **Tracker** transmits APRS position packets.
- **Trail logger** records GPS movement to microSD and does not transmit.

## APRS tracker

The Tracker page allows selection of:

- enabled or disabled periodic tracking
- GPS or saved default position
- normal or compressed APRS position
- fixed interval or SmartBeacon scheduling
- SmartBeacon profile: **Car**, **Bicycle** or **Walking**
- APRS symbol
- APRS path: `DIRECT`, `WIDE1-1` or `WIDE2-2`
- printable APRS comment, maximum 48 characters; compressed packets transmit the first 40 characters
- Trail logger enable state

### Available symbols

| Selection | APRS table/code |
|---|---|
| Car | `/>` |
| Pedestrian | `/[` |
| Bicycle | `/b` |
| Motorcycle | `/<` |
| Fixed QTH / house | `/-` |
| Boat | `/s` |
| Aircraft | `/^` |
| Balloon | `/O` |
| Weather station | `/_` |
| Generic point | `//` |
| LoRa iGate | `L&` |

### Manual BOOT beacon

A short BOOT-button press requests one position beacon even when periodic tracking is disabled. The packet uses the saved Tracker source, format, symbol, path and comment.

- GPS source: waits up to 15 seconds for a current fix
- Default source: uses saved coordinates immediately
- busy radio: request waits for the TX queue or times out

The first BOOT press after full display blanking only wakes the display.

## SmartBeacon

SmartBeacon changes the beacon interval according to speed and heading changes. Version 2.7.11 provides three tuned profiles:

| Profile | Low / high speed | Slow / fast interval | Movement behaviour |
|---|---:|---:|---|
| Car | 5 / 70 km/h | 30 min / 2 min | start after 8 s above 6 km/h; stop after 45 s below 3 km/h |
| Bicycle | 3 / 30 km/h | 20 min / 90 s | start after 6 s above 4 km/h; stop after 30 s below 2 km/h |
| Walking | 1.5 / 7 km/h | 15 min / 2 min | start after 10 s above 2 km/h; stop after 60 s below 0.7 km/h |

Each profile also changes corner sensitivity and minimum turn time. Course data at unreliable near-zero speed are ignored. Start and stop conditions must remain stable for the listed confirmation time, which prevents GPS jitter from generating extra packets.

The SmartBeacon timer is updated only after the radio confirms a completed RF transmission. Queueing a packet does not count as a transmission. Failed or unconfirmed packets are retried after a short delay.

The Tracker page shows the reason for the last completed beacon: initial, fixed interval, speed interval, corner, start moving, stopped or manual.

## Trail logger

Enable **Trail logger** on the Tracker page, then open **Trail logger** from the main menu.

A recording session starts only when:

- the feature is enabled
- a FAT32 microSD card is mounted
- GPS has a current fix
- GPS date and time are valid

Files are written to:

```text
/STOPAR/YYYYMMDD_HHMMSS.txt
```

Example data:

```text
# UTC;latitude;longitude;altitude_m;speed_kmh;course_deg;satellites;hdop;state
2026-08-02T18:32:10Z;49.786333;13.285000;324.0;4.2;180.0;9;0.9;RECORDING
```

## Point selection and autopause

Default behavior:

- evaluate every 5 seconds
- save after at least 3 metres
- force a point after 30 seconds
- automatic pause after 30 seconds without movement
- resume at speed of at least 2 km/h or movement of at least 8 metres

Manual pause takes priority over automatic pause.

## SD-card protection

The logger uses a fixed RAM queue and writes at most one queued line per main-loop pass. A slow or failing SD card may still delay one loop iteration. Use a quality card and do not remove it while recording.
