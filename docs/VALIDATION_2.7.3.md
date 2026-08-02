# Validation 2.7.3

## Host checks

```bash
g++ -std=gnu++17 -Wall -Wextra -Werror \
  -Itools/power_test_stubs -Isrc -Iinclude \
  tools/test_power_service.cpp src/services/power_service.cpp \
  src/app/localization.cpp -o test_power_service
./test_power_service
```

The test must confirm that the first valid battery reading creates one history
point with the detected discharging mode.

Run syntax checks for the modified UI and radio files with the existing host
stubs. The final firmware must also be built in PlatformIO:

```powershell
pio run -e waveshare-esp32-release
```

## Device validation

1. Open **Napajeni / Power** with a connected battery. The newest point must be
   shown at the right side of the chart.
2. Leave the terminal running for at least 30 minutes. A regular history point
   must be added every 15 minutes.
3. Connect USB-C. A new point must be added immediately and the following graph
   section must be blue or green if the PMIC reports active charging.
4. Disconnect USB-C. A new point must be added immediately and battery discharge
   must continue as an orange section.
5. Verify that the chart contains at most 96 points and gradually represents
   approximately 24 hours of operation.
6. Open **Diagnostika / Diagnostics**, change the LoRa frequency or RF profile,
   save it and wait until the profile is applied. The old RSSI history must be
   cleared and the displayed frequency must match the active setting.
7. Restart the terminal. The battery graph may start empty because version 2.7.3
   intentionally keeps this history only in RAM.
