# Validation checklist - firmware 2.6.0

## Scope

Version 2.6.0 extends the Czech/English selector from Tracker to the complete UI while
preserving NVS compatibility, radio configuration and APRS packet contents.

## Host checks completed

The following checks are intended to run with `-Wall -Wextra -Werror`:

- central localization behavior and revision counter
- NVS language persistence and invalid-value fallback
- all UI screen translation units and `ScreenManager` syntax
- power, display-power, time, GPS, Trail logger, map and DIGI/iGate services
- TX queue, notification events, geographic calculations and map projection
- APRS symbol lookup and map conversion Python syntax

## On-device acceptance test

1. Upgrade a device that already has version 2.5.0 and verify that the stored language
   is retained without clearing NVS.
2. Open Settings, select English and save.
3. Verify that the current page changes immediately without a restart.
4. Visit every menu entry and verify English labels, hints, statuses and errors.
5. Return to Settings, select Czech and save; repeat the screen review.
6. Restart the terminal and verify persistence of the selected language.
7. With GPS disconnected, SD removed and LoRa unavailable in separate tests, verify
   that error messages use the selected language.
8. Start and stop Tracker, Trail logger, DIGI and iGate and verify runtime status text.
9. Create a new Trail logger TXT file in each language and inspect its first comment
   line.
10. Verify that raw TNC2 frames, NMEA sentences, callsigns and APRS-IS replies are not
    modified by language switching.
11. Transmit and receive a known LoRa APRS packet and confirm that frequency, SF, BW,
    CR, sync word and disabled payload CRC are unchanged.

## Build verification still required

Run the complete target build in the local PlatformIO environment:

```powershell
pio run -e waveshare-esp32-release
```

After a successful build, review reported Flash and RAM use and perform the on-device
acceptance test above.
