# Web OTA update

Firmware v2.7.8 provides a temporary browser-based firmware updater on a local WPA2 access point.

## One-time USB installation

Version 2.7.8 changes the 16 MB Flash layout to two 7 MB A/B application slots:

```text
nvs       20 KB
otadata    8 KB
ota_0       7 MB
ota_1       7 MB
spiffs   1920 KB
coredump   64 KB
```

When upgrading from v2.7.7 or an older single-slot layout, perform one complete PlatformIO USB upload:

```powershell
pio run -e waveshare-esp32-release -t upload
```

Uploading only `firmware.bin` cannot install a new partition table. After the one-time USB installation, later compatible firmware can use Web OTA.

## Start OTA mode

1. Open **Settings** on the terminal.
2. Enable **Start web OTA mode** and press **Save**.
3. Connect a phone or computer to:

```text
SSID: LoRa-APRS-OTA
Password: loraaprs
```

4. Open `http://192.168.4.1`.
5. Select `.pio/build/waveshare-esp32-release/firmware.bin`.
6. Start the upload and keep the terminal powered.
7. Wait for the success message and automatic restart.

OTA mode is runtime-only and returns disabled after restart. When the APRS-IS iGate is active, Wi-Fi remains in AP+STA mode so the maintenance AP and the configured station connection can coexist.

## Browser functions

- displays the running firmware version
- displays the maximum accepted image size for the inactive OTA slot
- checks the `.bin` extension and file size before upload
- shows upload progress
- polls `/status` for device-side state
- provides **Stop OTA access point** through `POST /stop`
- restarts only after a successful `Update.end(true)`

## Firmware validation

Before any bytes are written to Flash, the updater buffers and checks the beginning of the file:

- ESP32 image magic must be `0xE9`
- segment count must be in the supported range
- Flash mode field must be valid
- ESP application descriptor magic must be `0xABCD5432`
- announced file size must fit the inactive OTA application slot

These checks reject common wrong uploads such as `bootloader.bin`, `partitions.bin`, filesystem images and random binary files. The Arduino Update library then performs its own image and write validation.

## Failure behaviour

- invalid images are rejected before Flash writing begins
- interrupted or short uploads call `Update.abort()`
- write failures do not schedule a restart
- the currently running firmware remains the active image after a rejected or interrupted upload
- OTA cannot repair firmware that crashes before the service starts; use USB in that case

## Security

The default credentials are stored in `include/app_config.h`. Change `OTA_AP_PASSWORD` before wider deployment. Keep OTA disabled outside maintenance and use a password of at least eight characters.

## Hardware validation checklist

After installing v2.7.8 through USB:

1. enable OTA and verify the AP at `192.168.4.1`
2. upload the same v2.7.8 `firmware.bin`
3. verify progress, success and automatic restart
4. confirm settings and persistent battery history survive the restart
5. test one invalid binary and one interrupted upload
6. if iGate is used, verify APRS-IS reconnection does not remove the OTA AP
