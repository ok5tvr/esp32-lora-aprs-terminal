# DIGI and iGate

The terminal provides two independent network functions:

- APRS RF digipeater
- receive-only RF-to-APRS-IS iGate

They may be enabled separately or together.

## Digipeater modes

Available modes:

- `WIDE1-1 fill-in`
- `WIDE2-N trace`
- `WIDE1 + WIDE2`

The implementation processes only the first unused path component. It does not skip an unsupported first request to repeat a later one.

Example for local callsign `OK5TVR-17`:

```text
Input:  OK1ABC>APRS,WIDE1-1,WIDE2-1:...
Output: OK1ABC>APRS,OK5TVR-17*,WIDE2-1:...
```

For WIDE2:

```text
Input:  OK1ABC>APRS,WIDE2-2:...
Output: OK1ABC>APRS,OK5TVR-17*,WIDE2-1:...
```

## Packets not repeated

A packet is rejected from digipeating when, for example:

- its source is the local callsign
- the local callsign is already marked used
- the frame contains `TCPIP`, `TCPXX` or an APRS-IS q construct
- the first unused path element is unsupported
- the requested WIDE2 value exceeds the configured local maximum
- an equivalent packet was heard during the duplicate-suppression interval

Eligible repeats use a small random delay before entering the central TX queue.

## Receive-only iGate

The iGate sends eligible LoRa RF packets to APRS-IS using `qAO`.

```text
RF: OK1ABC>APRS,WIDE1-1:>status
IS: OK1ABC>APRS,WIDE1-1,qAO,OK5TVR-17:>status
```

Traffic from APRS-IS back to RF is intentionally disabled.

The iGate does not forward:

- generic queries starting with `?`
- packets containing `NOGATE` or `RFONLY`
- packets containing `TCPIP`, `TCPXX` or an APRS-IS q construct
- Internet-origin third-party packets
- malformed TNC2 data

Packets are sent only after the APRS-IS server reports a verified login.

## Configuration

Open **DIGI / iGate** and configure:

- Digipeater RF-to-RF switch
- DIGI mode
- maximum WIDE2 request: 1 or 2
- RX iGate RF-to-IS switch
- Wi-Fi SSID and password
- APRS-IS server and port
- valid APRS-IS passcode
- optional server filter

Default server:

```text
rotate.aprs2.net:14580
```

Example filter expression:

```text
r/49.78/13.28/50
```

Enter only the expression. The firmware adds the `filter` keyword to the login line.

## Recommended modes

| Intended use | Digipeater | RX iGate |
|---|---:|---:|
| Passive terminal | Off | Off |
| Receive-only iGate | Off | On |
| Digipeater only | On | Off |
| Combined site | On | On |

Disable periodic Tracker transmissions at a fixed infrastructure site unless position beacons are intentionally required. Incoming directed messages may still trigger automatic ACK packets.

## Security note

Wi-Fi credentials and APRS-IS passcode are stored in ordinary NVS. They are not encrypted unless ESP32 Flash encryption is configured separately.
