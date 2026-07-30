# Validation checklist - version 1.1.0 Stopar

## Static and host checks

- Build `trail_service.cpp`, `trail_screen.cpp`, modified Tracker/Menu/ScreenManager/AppController/SettingsService sources.
- Run existing host tests for geometry, DIGI/iGate and notification events.
- Run the mock-SD TrailService test for file creation, data/event rows, autopause, automatic resume, manual pause/resume and clean close.

Example host command from the project root:

```bash
g++ -std=gnu++17 -Wall -Wextra \
  -Itools/trail_test_stubs -Iinclude -Isrc \
  tools/test_trail_service.cpp \
  src/services/trail_service.cpp src/services/geo_utils.cpp \
  -o test_trail_service
./test_trail_service
```

## Device functional test

1. Start without an SD card. Verify Stopar cannot be enabled and radio receive remains functional.
2. Start with a FAT32 SD card and Stopar disabled. Verify the main save icon is grey.
3. Enable Stopar on Tracker and save. Verify amber waiting state until GPS fix and valid UTC are available.
4. Obtain a GPS fix. Verify a new `/STOPAR/YYYYMMDD_HHMMSS.txt` file and a green recording icon.
5. Walk or drive for at least 2 minutes. Verify point count and distance increase and APRS reception continues.
6. Stop for more than 30 seconds. Verify automatic pause and an `AUTO_PAUSE` event.
7. Move faster than 2 km/h or more than 8 metres. Verify automatic resume and an `AUTO_RESUME` event.
8. Press **Pozastavit**, move, and verify no route points are added. Press **Pokracovat** and verify logging resumes.
9. Disable Stopar on Tracker. Verify the file closes, `STOP` is written and the log appears in the newest-first list.
10. Reboot with Stopar enabled. Verify a new session starts only after GPS date/time and fix are valid.
11. Receive APRS traffic and trigger tracker beacons while recording. Verify packets are not lost systematically and scheduled beacons remain on time.
12. Inspect the TXT file on a computer. Verify semicolon-separated columns, UTC timestamps and readable event comments.

## Stress and fault test

- Record for at least one hour with tracker, DIGI and receive-only iGate enabled.
- Confirm the Stopar dropped-line counter remains zero.
- Remove the card only after pausing/disabling; verify no filesystem corruption.
- Repeat with a deliberately slow card and note any UI latency. Do not accept a card that produces write errors or dropped lines.
- Fill the card near capacity and verify a failed write produces the red Error state without repeated file creation.
