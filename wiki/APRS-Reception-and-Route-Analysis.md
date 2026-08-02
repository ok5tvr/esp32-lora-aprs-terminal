# APRS Reception and Route Analysis

## Supported received entities

The terminal decodes and stores the 15 most recently heard unique APRS:

- stations
- objects
- items
- weather stations

Supported position formats include normal uncompressed position, Base-91 compressed position and Mic-E. The parser also recognizes weather fields, classic telemetry, PHG, frequency/tone/offset information and emergency states.

## Received stations list

Use **Up** and **Down** to select a station. Press **OK** to open its detail.

The station detail may contain:

- source callsign and SSID
- APRS symbol
- last-heard age
- packet count
- position and format
- RSSI and SNR
- weather or telemetry fields
- frequency, tone and offset information
- emergency state
- complete stored TNC2 frame
- complete APRS path analysis

## DIRECT versus VIA DIGI

The parser keeps the complete path after the APRS destination. Path components ending in `*` are considered used digipeater elements.

Example direct reception:

```text
OK1ABC-7>APRS,WIDE1-1,WIDE2-1:!....
```

No path component is marked used, so the reception is shown as:

```text
DIRECT | hops 0
```

Example repeated reception:

```text
OK1ABC-7>APRS,OK0AAA-2*,WIDE2-1*:!....
```

The detail shows:

```text
VIA DIGI | hops 2 | last OK0AAA-2 or the rightmost used RF element
```

The firmware stores:

- full latest path
- number of used RF path elements
- last used digipeater
- direct reception count
- repeated reception count
- age of the last direct reception

Internet-only path elements such as `TCPIP`, `TCPXX` and APRS-IS `qA` constructs are not counted as RF hops. For a one-level third-party packet beginning with `}`, the inner original frame is analyzed.

## How to interpret route information

- **DIRECT** means that no RF digipeater element was marked used in the received frame.
- **VIA DIGI** means at least one RF path element was already consumed.
- A requested but unused path such as `WIDE1-1,WIDE2-1` does not by itself mean the packet was repeated.
- The last used digipeater is the rightmost used RF path component.

## Navigation to a station

If the selected entity has a valid position, press **OK** from its detail to open navigation. The screen shows live distance and true bearing from:

1. current GPS position, when available
2. saved default position otherwise

## Storage behavior

Station history is kept in fixed RAM. It is cleared after reboot. The newest entity is first; an updated callsign is moved to the front. When the 15-entry capacity is full, the oldest unique entity is discarded.
