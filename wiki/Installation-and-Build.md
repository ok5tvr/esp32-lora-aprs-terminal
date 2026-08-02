# Installation and Build

## Requirements

- Visual Studio Code with PlatformIO, or PlatformIO Core
- USB driver appropriate for the board
- Git for source control and optional Wiki publishing
- Python 3 for map conversion and host tools

## PlatformIO environments

The normal release environment is:

```text
waveshare-esp32-release
```

Build:

```powershell
pio run -e waveshare-esp32-release
```

Upload through USB:

```powershell
pio run -e waveshare-esp32-release -t upload
```

Open the serial monitor:

```powershell
pio device monitor -b 115200
```

## Clean build

After changing board families or replacing an older ESP32-S3 project, delete the `.pio` folder or run:

```powershell
pio run -t clean
```

Then rebuild.

## Output firmware file

The Web OTA updater uses:

```text
.pio/build/waveshare-esp32-release/firmware.bin
```

Do not upload a filesystem image, bootloader image or partition-table binary through the OTA page.

## OTA partition layout

Firmware v2.7.5 and later uses two approximately 7 MB application slots in the 16 MB Flash partition table.

When upgrading from a build that used the previous single factory application partition, perform one complete USB upload first. Uploading only `firmware.bin` cannot change the partition table.

## Recommended first validation

After upload, verify the serial log and then test:

1. display and touch
2. AXP2101 power telemetry
3. microSD detection
4. GPS traffic
5. RA-02 initialization
6. LoRa reception
7. station detail and route analysis
8. tracker/manual beacon
9. Web OTA

## Native tests

The `tools` directory contains host-side tests for APRS parsing, messages, route analysis, map projection, power history and other logic. These tests improve regression coverage but do not replace a full target build and hardware test.
