# Web OTA Update

Firmware v2.7.8 includes a temporary, validated A/B Web OTA mode.

> [!IMPORTANT]
> Install v2.7.8 once through the complete PlatformIO USB Upload action. The release changes the Flash partition table; uploading only `firmware.bin` cannot create the two OTA slots.

## First OTA-capable installation

```powershell
pio run -e waveshare-esp32-release -t upload
```

The 16 MB layout then contains two 7 MB application slots. Later compatible releases can update the inactive slot through the browser.

## Start OTA mode

1. Open **Settings**.
2. Enable **Start web OTA mode**.
3. Press **Save**.
4. Connect a phone or computer to:

```text
SSID: LoRa-APRS-OTA
Password: loraaprs
```

5. Open `http://192.168.4.1`.
6. Select `.pio/build/waveshare-esp32-release/firmware.bin`.
7. Start the upload and keep the terminal powered.
8. Wait for success and automatic restart.

OTA is runtime-only and turns off after restart. When iGate is enabled, AP+STA mode keeps both the maintenance AP and the configured APRS-IS Wi-Fi connection active.

## Web page functions

- current firmware version and maximum image size
- browser-side `.bin` and file-size checks
- upload progress
- live device status through `/status`
- safe **Stop OTA access point** action through `/stop`
- automatic restart only after a verified successful write

## Validation and safety

The device checks the buffered beginning of the upload before writing:

- ESP32 image magic `0xE9`
- supported segment count
- valid Flash-mode field
- ESP application descriptor magic `0xABCD5432`
- image size within the inactive OTA slot

This rejects bootloader, partition-table, filesystem and unrelated binary files. The Arduino Update layer performs additional validation during and after writing.

An invalid, oversized, short or interrupted upload is aborted and does not schedule a restart. The currently running firmware remains active.

## Security

Change `OTA_AP_PASSWORD` in `include/app_config.h` before wider deployment. Enable OTA only during maintenance.

## Recovery

Use USB when:

- the installed firmware crashes before OTA initialization
- the old installation has no A/B OTA partition table
- the OTA AP cannot be reached
- a target build or partition mismatch is suspected
