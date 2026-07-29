# Validation notes - version 1.0.0

## Host tests

The hardware-independent APRS codec was compiled as C++17 with `-Wall`,
`-Wextra`, `-Wpedantic` and `-Werror`. Tests cover:

- `WIDE1-1` fill-in replacement
- traceable `WIDE2-2` decrement
- directed local-callsign repetition
- rejection of an already-used local path
- rejection of excessive WIDE2 requests
- receive-only `qAO` construction
- `NOGATE` and generic-query filtering
- third-party unwrapping
- rejection of Internet-origin inner and outer third-party paths
- path-independent duplicate hashing
- APRS-IS output capacity for the appended `qAO` path

The DIGI/iGate service was also compiled with host Arduino/WiFi stubs and tested
for delayed queue release, duplicate suppression, successful-repeat counters,
iGate queueing and `NOGATE` filtering. The implementation also discards gated
packets that remain queued for more than 30 seconds during a network outage.

## Syntax checks

The changed settings, network service, radio service, application controller,
screen manager and DIGI/iGate LVGL screen were syntax-checked as C++17 with
strict warnings using local API stubs.

A complete ESP32 PlatformIO link was not available in the build environment. A
clean PlatformIO build on the target workstation is still required.

## Runtime checks recommended

1. Start with both services disabled.
2. Verify receive and transmit operation on the existing LoRa profile.
3. Enable only WIDE1 fill-in and send one controlled `WIDE1-1` test packet.
4. Verify that a repeated copy within 30 seconds is not transmitted again.
5. Enable WIDE2 with maximum 1 before testing maximum 2.
6. Configure WiFi and APRS-IS while the iGate remains disabled.
7. Enable the iGate and verify `VERIFIED` on the display.
8. Confirm the gated line on APRS-IS contains `qAO,<IGATECALL>`.
9. Send controlled packets with `NOGATE`, `RFONLY` and `TCPIP`; confirm the
   filtered counter increases and no line is sent.
10. Monitor RF airtime and disable DIGI if the local channel becomes congested.
