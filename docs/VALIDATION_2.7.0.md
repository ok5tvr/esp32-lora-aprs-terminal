# Validation checklist - firmware 2.7.0

## Menu and localization

Czech and English menu order:

1. LoRa APRS
2. Prijate stanice / Received stations
3. Zpravy / Messages
4. Meteostanice / Weather stations
5. Mapa / Map
6. Tracker
7. Stopar / Trail logger
8. DIGI / iGate
9. GPS prijimac / GPS receiver
10. Astronomie / Astronomy
11. Diagnostika / Diagnostics
12. Napajeni / Power
13. Nastaveni / Settings

Verify that changing the interface language rebuilds the Astronomy screen without restart.

## Time and position sources

- Start without GPS fix but with valid RTC and verify that the page uses the saved default position.
- Obtain a GPS fix and verify that the source changes to GPS.
- Remove the GPS antenna/fix and verify fallback to the default position.
- Start with invalid RTC and no GPS time; the page must show that it is waiting for valid time and position.
- After GPS date/time arrives, verify automatic calculation without reboot.

## Astronomical values

For Pilsen near 49.7863 N, 13.2850 E on 2026-07-31 the host calculation should be approximately:

- sunrise around 05:35 CEST
- sunset around 20:50 CEST
- daylight around 15 h 15 min
- Moon close to full, illumination above 90 percent

A few minutes difference is acceptable because the firmware uses compact low-precision ephemerides.

## Runtime behavior

- Confirm no repeated calculation while date and position are unchanged.
- Confirm recalculation after local midnight.
- Confirm recalculation after moving more than 5 km.
- Confirm no effect on LoRa RX/TX, Tracker, DIGI/iGate, Stopar or map loading.
