# Web OTA update

## Activation

1. Open **Settings** on the terminal.
2. Enable **OTA update** and press **Save**.
3. Connect a phone or computer to Wi-Fi `LoRa-APRS-OTA`.
4. Use password `loraaprs`.
5. Open `http://192.168.4.1`.
6. Select the PlatformIO output file `.pio/build/waveshare-esp32-release/firmware.bin`.
7. Keep the terminal powered until the page confirms success and the device restarts.

OTA mode is runtime-only and is disabled after restart. If iGate is enabled,
the firmware uses AP+STA mode so the maintenance access point and the configured
Wi-Fi station can coexist.

## First installation

Version 2.7.5 changes `partitions_16mb.csv` from a single factory application
partition to two 7 MB OTA slots. Install 2.7.5 once by USB using the complete
PlatformIO **Upload** action. Flashing only `firmware.bin` onto an older partition
layout is not sufficient and the web updater will report that no OTA partition
is available.

## Validation and safety

- Only `.bin` files are accepted.
- The first image byte must match the ESP32 application image header (`0xE9`).
- Arduino `Update` validates the image before selecting the new boot partition.
- A failed, interrupted, or invalid upload does not reboot the terminal.
- The browser updater is reachable only through the local WPA2 access point.
- Change `OTA_AP_PASSWORD` in `include/app_config.h` before wider deployment if
  a project-specific password is preferred.
