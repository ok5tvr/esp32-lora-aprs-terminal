# Validation checklist - version 1.2.0 power management

## Completed static and host checks

- `PowerService` compiles with strict `-Wall -Wextra -Werror` against an AXP2101 API stub.
- Header power rendering and the Power page compile with strict warnings against an LVGL 8 API stub.
- Existing geometry, DIGI/iGate core, notification-event and Stopar host tests pass.
- The service contains no setter for charge current, target voltage or PMIC rails.
- Power polling remains after GPS, radio, tracker and Stopar processing in `AppController::update()`.

Reproducible host command:

```bash
g++ -std=gnu++17 -Wall -Wextra -Werror \
  -Itools/power_test_stubs -Iinclude -Isrc \
  tools/test_power_service.cpp src/services/power_service.cpp \
  -o test_power_service
./test_power_service
```

A complete embedded PlatformIO build still needs to be run on the development
computer because PlatformIO is not available in the validation container.

## Device functional test

1. Start on battery only. Verify a battery symbol, plausible percentage and battery voltage in the header.
2. Open **Napajeni**. Verify `Akumulator: pripojen` and a discharging/standby state.
3. Connect USB-C. Within two seconds verify the last event reports USB connection.
4. During charging verify a green lightning symbol and a plausible charger phase.
5. After charging completes verify a blue USB symbol instead of the lightning symbol.
6. Compare battery, VBUS and system voltages with the official Waveshare AXP2101 example.
7. Verify configured charging current and target voltage are displayed but unchanged after reboot.
8. Disconnect USB-C and verify the event and battery symbol update within two seconds.
9. Exercise LoRa reception and APRS tracker transmission while repeatedly connecting/disconnecting USB-C. Verify no systematic lost packets or delayed beacons.
10. Run for at least one hour and verify PMIC values remain plausible and the UI does not leak memory.

## Critical-state test

Testing a real Li-Pol cell below the safe limit is not recommended merely to
exercise the UI. For a bench test, temporarily raise
`POWER_CRITICAL_PERCENT` or `POWER_CRITICAL_VOLTAGE_MV`, rebuild, and verify the
header changes to red. Restore the production thresholds afterwards.
