# Web OTA Update

Firmware v2.7.5 and later includes a temporary browser-based OTA mode.

## Start OTA mode

1. Open **Settings**.
2. Enable **Start web OTA mode**.
3. Press **Save**.
4. Connect a phone or computer to Wi-Fi:

```text
SSID: LoRa-APRS-OTA
Password: loraaprs
```

5. Open:

```text
http://192.168.4.1
```

6. Select:

```text
.pio/build/waveshare-esp32-release/firmware.bin
```

7. Start the upload and keep the terminal powered until success is confirmed.
8. The terminal restarts automatically.

OTA mode is runtime-only and turns off after restart.

## First OTA-capable installation

The OTA partition table contains two application slots. When upgrading from an older partition layout, perform one complete USB upload first:

```powershell
pio run -e waveshare-esp32-release -t upload
```

Flashing only `firmware.bin` cannot install a new partition table.

## Validation and safety

- only `.bin` files are accepted
- the first image byte must match the ESP32 image header
- the Arduino Update library validates the image
- an invalid or interrupted upload does not intentionally select the new image
- the update page is reachable only through the local WPA2 access point
- when iGate is enabled, Wi-Fi uses AP+STA mode

## Security

The default OTA password is stored in `include/app_config.h`.

Change it before wider deployment. Use a strong password of at least eight characters. The OTA access point should be enabled only during maintenance.

## Recovery

If the terminal crashes before the OTA service starts, update it through USB. Web OTA cannot repair firmware that never reaches the OTA initialization stage.
