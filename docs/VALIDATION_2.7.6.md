# Validation 2.7.6 - APRS route analysis

## Host checks

Compile and run the route/parser plus station-store aggregation test:

```bash
g++ -std=gnu++17 -Wall -Wextra -Werror \
  -Ilib/AprsCore/src -Isrc -Iinclude \
  tools/test_station_route_analysis.cpp \
  src/services/station_store.cpp lib/AprsCore/src/aprs_codec.cpp \
  -o test_station_route_analysis
./test_station_route_analysis
```

The test verifies:

- a packet with an unused `WIDE1-1,WIDE2-1` path is classified as DIRECT;
- used path elements ending in `*` are counted as DIGI hops;
- the rightmost used element is retained as the last digipeater;
- the complete path is retained;
- direct and repeated counters accumulate independently for one station;
- the last-direct timestamp is not overwritten by a repeated packet;
- a later direct packet updates the last-direct timestamp and clears the current last-DIGI/path values when the packet has no path.

Check the expanded station-detail screen syntax:

```bash
g++ -std=gnu++17 -Wall -Wextra -Werror -fsyntax-only \
  -Itools/ui_test_stubs -Itools/gps_test_stubs -Itools/time_test_stubs \
  -Ilib/AprsCore/src -Isrc -Iinclude \
  src/ui/screens/station_detail_screen.cpp
```

The final embedded firmware must also be built in PlatformIO:

```powershell
pio run -e waveshare-esp32-release
```

## Device validation

1. Receive a packet directly with `WIDE1-1,WIDE2-1`. Detail must show `DIRECT`, zero hops and the complete unused path.
2. Receive the same station through one DIGI, for example `OK0ABC-2*,WIDE2-1`. Detail must show one hop and `OK0ABC-2` as the last DIGI.
3. Receive a two-hop path and verify that both used elements are counted and the rightmost one is shown as last.
4. Verify that direct and repeated counters increment separately.
5. After a repeated packet, verify that last-direct age continues from the most recent direct reception.
6. Use Up/Down to scroll through route details and the full TNC2 frame.
7. Confirm that OK still opens navigation for entities with a valid position.
