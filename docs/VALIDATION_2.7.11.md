# Validation 2.7.11

## Scope

Version 2.7.11 validates APRS position encoding, SmartBeacon profiles and completed-transmission scheduling.

## Host tests

The following checks passed:

- APRS core encode/decode tests for standard and Base-91 compressed positions
- exact `=` data identifier for messaging-capable position packets
- compressed comment truncation to exactly 40 characters
- TX queue sequence creation and preservation when a queued Tracker frame is replaced
- Settings NVS save/reload for the selected SmartBeacon profile
- profile interval calculations for car, bicycle and walking
- confirmed start-moving and stopped transitions
- corner calculation across north (`359° -> 2°`)
- Tracker, RadioService and Tracker UI syntax checks with host stubs
- existing OTA, route-analysis, power-history and settings tests

## Hardware verification

1. Install v2.7.11 and select **Tracker -> SmartBeacon**.
2. Test each profile: Car, Bicycle and Walking.
3. Confirm the first packet is transmitted after the normal start delay.
4. Confirm the countdown restarts only after the LoRa screen increments completed TX.
5. Temporarily disconnect or disable the radio and confirm the Tracker reports a failed/unconfirmed TX and retries later.
6. With the device stationary, confirm GPS heading changes do not trigger corner packets.
7. Move above the profile start threshold for the confirmation period and verify a `Start moving` beacon.
8. Stop below the profile stop threshold for the confirmation period and verify one `Stopped` beacon.
9. Drive or walk through a real turn and verify `Corner` appears as the last completed reason.
10. Select compressed format and a 48-character comment. Verify only the first 40 comment characters are transmitted and the packet still decodes correctly.
11. Confirm standard format retains the complete 48-character comment.
12. Press BOOT once and verify exactly one request and one completed manual beacon.
