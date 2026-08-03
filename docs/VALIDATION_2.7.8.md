# Validation record - firmware 2.7.8

## Scope

Stabilization release covering system diagnostics, APRS route badges, Internet-route classification, A/B Web OTA and restart-reason reporting.

## Static and host-side checks

| Area | Check | Result |
|---|---|---|
| APRS parser | Existing APRS native test suite | Passed |
| Route analysis | Direct, repeated and Internet-routed station aggregation | Passed |
| Route UI | Stations list and station detail syntax with LVGL stubs | Passed |
| System diagnostics | Service syntax with Arduino/ESP-IDF stubs | Passed |
| Screen integration | ScreenManager and Diagnostics screen syntax | Passed |
| OTA validation | Valid application header and rejection of malformed/wrong images | Passed |
| OTA layout | Partition offsets, overlap, bounds, two equal OTA slots | Passed |
| OTA service | WebServer/Wi-Fi/Update integration syntax with host stubs | Passed |
| Stack regression | `StationStore::clear()` remains free of the former large temporary object | Passed |

## OTA safety cases covered by host tests

- valid ESP32 application image prefix accepted
- wrong image magic rejected
- invalid segment count rejected
- invalid Flash mode rejected
- missing/wrong ESP application descriptor rejected
- filename extension validation
- 16 MB partition table contains `otadata`, `ota_0` and `ota_1`
- both OTA application slots are 7 MB and do not overlap other partitions

## Hardware checks still required

A complete target build and hardware Web OTA cycle could not be executed in the current environment because PlatformIO and ESP32 hardware are unavailable. Before release deployment, verify:

1. complete USB upload installs the new partition table
2. normal boot and all peripherals initialize
3. Diagnostics values update and reset reason is correct
4. D/1-9+/I/? badges match real received packets
5. valid OTA upload completes and boots the new slot
6. invalid and interrupted uploads retain the previous firmware
7. APRS-IS reconnect preserves the OTA AP in AP+STA mode
8. settings, NVS power history and microSD data survive OTA

## Upgrade limitation

The first v2.7.8 installation from an older single-slot build must use the complete PlatformIO USB Upload action. Web OTA cannot replace the partition table used by the currently installed firmware.
