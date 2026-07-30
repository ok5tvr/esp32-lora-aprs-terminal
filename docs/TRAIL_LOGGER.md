# Stopar GPS route logger

Version 1.1.0 adds an independent GPS route logger named **Stopar**.

## User workflow

1. Insert a working FAT32 microSD card before startup.
2. Open **Tracker** and switch **Stopar** on, then save the tracker settings.
3. Open **Stopar** from the main menu to watch the recording state.
4. Use **Pozastavit** to pause manually and **Pokracovat** to resume.
5. Disable Stopar on the Tracker page to close the current file cleanly.

The enable switch is persistent. A manual pause is deliberately not persistent;
a reboot starts from the saved enabled/disabled setting and waits for GPS again.

## Session and file format

A recording session starts only when all of these conditions are true:

- Stopar is enabled,
- the microSD card is mounted,
- GPS has a current position fix,
- GPS date and time are valid.

Files are stored in `/STOPAR` and use GPS UTC in the filename:

```text
/STOPAR/20260729_105812.txt
```

If the filename already exists, `_01` through `_99` is appended. Each file is
plain UTF-8/ASCII-compatible text with semicolon-separated values:

```text
# LoRa APRS Terminal - Stopar
# UTC;latitude;longitude;altitude_m;speed_kmh;course_deg;satellites;hdop;state
# EVENT;2026-07-29T10:58:12Z;START
2026-07-29T10:58:12Z;49.786333;13.285000;324.0;0.0;0.0;9;0.9;RECORDING
```

The Stopar page lists the eight newest `.txt` files and their sizes. Filenames
sort chronologically because they begin with `YYYYMMDD_HHMMSS`.

## Point selection

Default constants are in `include/app_config.h`:

- evaluate recording every 5 seconds,
- save a new point after at least 3 metres,
- force a point after 30 seconds even below the distance threshold,
- ignore a single distance jump greater than 2 km in the accumulated distance.

The 3 metre filter limits GPS jitter and unnecessary SD writes. The forced
30-second point preserves a coarse trace during very slow movement.

## Automatic pause

Autopause begins after 30 seconds without detected movement. It resumes when
one of these conditions is met:

- valid GPS speed is at least 2 km/h,
- displacement from the stationary reference is at least 8 metres.

Automatic pause and resume are written as event comments. Manual pause always
takes precedence over automatic pause. After a manual resume, automatic pause
starts evaluating movement again from a fresh reference.

## Runtime and performance

Stopar does not change APRS tracker intervals and does not transmit packets.
The main loop handles GPS, LoRa and APRS tracker work before calling Stopar.

Storage protection consists of:

- a fixed queue of 12 text lines in RAM,
- at most one queued line written per loop,
- a normal 5-second sampling interval,
- `flush()` after eight lines or 15 seconds, only with an empty queue,
- a dropped-line counter shown on the Stopar page if the queue ever fills.

LoRa is connected to HSPI, separately from the LCD/microSD VSPI bus. Therefore
normal route logging should not noticeably affect LoRa reception or tracker
operation. ESP32 SD library calls are synchronous, however. A slow, failing or
fragmented card can block a loop iteration briefly. Use a reputable FAT32 card,
avoid removing it while recording, and replace it if the dropped-line counter
or SD errors appear.

## Error behaviour

A write or directory error changes the Stopar state to red **Error** and closes
the affected file where possible. The error is latched so the service does not
repeatedly create files or retry a failing card. Disable and re-enable Stopar
after correcting the card problem.
