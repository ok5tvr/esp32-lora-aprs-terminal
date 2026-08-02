# Validation 2.7.4

## Host checks

Compile and run the power-service test with NVS and AXP2101 stubs:

```bash
g++ -std=gnu++17 -Wall -Wextra -Werror \
  -Itools/power_test_stubs -Isrc -Iinclude \
  tools/test_power_service.cpp src/services/power_service.cpp \
  src/app/localization.cpp -o test_power_service
./test_power_service
```

The test verifies:

- the first valid battery value creates one history point;
- an unchanged two-second poll does not write another point;
- a 1-percent decrease is recorded after two consecutive confirming polls;
- a charging/discharging mode transition is recorded immediately;
- a stable value is checkpointed after one hour;
- a new `PowerService` instance restores the NVS history after a simulated reset;
- restart does not create a duplicate point when percentage and mode are unchanged;
- a new 1-percent change continues after the restored timeline.

Check the Power screen syntax with the LVGL host stubs:

```bash
g++ -std=gnu++17 -Wall -Wextra -Werror -fsyntax-only \
  -Itools/ui_test_stubs -Itools/gps_test_stubs -Itools/time_test_stubs \
  -Isrc -Iinclude src/ui/screens/power_screen.cpp
```

The final embedded firmware must also be built in PlatformIO:

```powershell
pio run -e waveshare-esp32-release
```

## Device validation

1. Start with a connected battery and open **Napajeni / Power**. One current
   point must be visible at the right side of the graph.
2. Let the battery decrease by 1 percent. After the value remains equal for two
   consecutive polls, a new orange point/segment must be added without waiting
   for a fixed interval.
3. Connect USB-C. A point must be added immediately and the following section
   must be green while charging or blue while supplied from USB.
4. Leave the percentage unchanged for at least one hour. A checkpoint point
   must be added.
5. Restart the terminal. The same history and elapsed-time span must reappear;
   no duplicate startup point should be added when percentage and mode match.
6. Power the terminal completely off and on. History must still be restored
   from NVS.
7. Fill more than 96 points. The oldest point must be discarded while the most
   recent 96 points remain available after restart.
8. Confirm that normal two-second telemetry polling does not cause continuous
   NVS writes or visible UI pauses.
