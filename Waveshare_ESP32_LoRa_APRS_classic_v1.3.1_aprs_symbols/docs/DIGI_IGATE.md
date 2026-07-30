# DIGI and receive-only APRS-IS iGate

Firmware 1.0.0 adds a separate **DIGI / iGate** page. Both services can be
enabled independently and their startup state is stored in ESP32 NVS.

## Digipeater modes

Available modes are:

- `WIDE1-1 fill-in`
- `WIDE2-N trace`
- `WIDE1 + WIDE2`

The implementation examines only the first unused path component. It never
skips an unsupported first unused component to repeat a later request. This
keeps the path processing deterministic and avoids bypassing path restrictions.

Examples for local call `OK5TVR-17`:

```text
OK1ABC>APRS,WIDE1-1,WIDE2-1:...
OK1ABC>APRS,OK5TVR-17*,WIDE2-1:...

OK1ABC>APRS,WIDE2-2:...
OK1ABC>APRS,OK5TVR-17*,WIDE2-1:...

OK1ABC>APRS,OK5TVR-17:...
OK1ABC>APRS,OK5TVR-17*:...
```

The configured WIDE2 maximum can be one or two. Requests above that local
maximum are not repeated. Obsolete generic aliases such as `RELAY`, `WIDE` and
`TRACE` are not supported.

A packet is not repeated when:

- its source is the local callsign
- the local callsign is already marked as used in the path
- it already contains `TCPIP`, `TCPXX` or an APRS-IS q construct
- the first unused path element does not match the selected mode
- the WIDE2 request exceeds the configured local maximum
- the same source, destination and information field was heard within 30 seconds

Eligible repeats are delayed by a random 120-420 ms. The queue contains four
fixed-size entries. APRS messages have transmit priority, followed by a due
digipeater packet and then the tracker.

## Receive-only iGate

This version gates only from LoRa RF to APRS-IS. Internet-to-RF message gating
is deliberately disabled. A receive-only iGate uses the `qAO` construct:

```text
RF input:
OK1ABC>APRS,WIDE1-1:>status

APRS-IS output:
OK1ABC>APRS,WIDE1-1,qAO,OK5TVR-17:>status
```

The iGate does not forward:

- generic APRS queries whose information field starts with `?`
- packets with `NOGATE` or `RFONLY`
- packets with `TCPIP` or `TCPXX`
- packets that already contain an APRS-IS q construct
- third-party packets whose outer or inner path indicates Internet origin
- malformed TNC2 lines or data containing CR, LF or NUL

An eligible one-level third-party packet is unwrapped before it is sent to
APRS-IS. The iGate keeps an eight-entry fixed queue and applies the same
30-second path-independent duplicate interval. Frames are sent only after the
APRS-IS server reports a verified login. Queued packets older than 30 seconds
are discarded after an outage instead of being uploaded as a stale burst.
The APRS-IS line buffer includes space for the appended `qAO,<IGATECALL>` path
and enforces the 512-byte limit including CR/LF.

## Configuration fields

- Digipeater enable
- DIGI mode
- maximum WIDE2 request: one or two hops
- receive-only iGate enable
- WiFi SSID and password
- APRS-IS server and TCP port
- APRS-IS passcode valid for the operating callsign
- RF/APRS-IS callsign validation (3-6 base characters, optional SSID 1-15,
  never explicit `-0` when iGate is enabled)
- optional APRS-IS server filter expression

The default server is `rotate.aprs2.net` on port `14580`. Enter only the filter
expression, for example `r/49.78/13.28/50`; the login generator adds the word
`filter`.

The page displays WiFi and APRS-IS state, IP address, verified-login state,
queue depths and packet/filter/duplicate counters.

## Important limitations

The LoRa network transports a TNC2-like APRS payload, not a complete AX.25 UI
frame. Therefore the firmware cannot validate AX.25 control, PID or FCS fields
that are not present in the received payload. It validates the TNC2 structure,
path rules and prohibited APRS-IS markers. LoRa payload CRC remains disabled
for compatibility with the deployed LoRa APRS profile.

Internet-to-RF operation is not included. A compliant bidirectional iGate would
need to track recently heard stations, gate only narrowly eligible messages,
construct third-party RF packets, limit RF coverage and prevent ACK/message
loops.

WiFi credentials and the APRS-IS passcode are stored in normal NVS. They are
not encrypted unless ESP32 Flash encryption is configured separately.

## Quick mode selection

- Passive terminal only: digipeater OFF, RX iGate OFF.
- Receive-only iGate: digipeater OFF, RX iGate ON. Configure WiFi, APRS-IS server, port and a verified passcode.
- Digipeater only: digipeater ON, RX iGate OFF. Select the WIDE mode appropriate for the site.
- Combined digipeater and receive-only iGate: both switches ON.

Tracker and APRS messaging are independent services. Disable the tracker for a station that must not transmit periodic beacons. Incoming APRS messages addressed to the local callsign may still cause an automatic ACK transmission.
