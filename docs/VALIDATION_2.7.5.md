# Validation 2.7.5 - Web OTA

## Static checks

- Settings handler carries the runtime OTA-enabled flag from LVGL to AppController.
- OtaService starts `LoRa-APRS-OTA` on `192.168.4.1` and serves `/` plus `/update`.
- iGate enabled selects `WIFI_AP_STA`; iGate disabled selects `WIFI_AP`.
- Disabling OTA preserves station mode when iGate remains enabled.
- Upload accepts `.bin`, checks the ESP32 `0xE9` image header, and calls
  `Update.begin`, `Update.write`, and `Update.end(true)`.
- Restart is scheduled only after a successful upload.
- OTA is not stored in NVS and therefore returns disabled after reboot.

## Partition checks

- `otadata` starts at `0xE000` and ends at the application boundary `0x10000`.
- `ota_0` occupies `0x10000-0x70FFFF`.
- `ota_1` occupies `0x710000-0xE0FFFF`.
- SPIFFS and coredump end at the 16 MB flash boundary.

## Hardware test required

1. Install 2.7.5 by full USB upload.
2. Enable OTA and verify association with the WPA2 AP.
3. Open `192.168.4.1` and upload a newer `firmware.bin`.
4. Confirm restart into the new image.
5. Interrupt a second upload and confirm the previous image still boots.
6. Repeat with iGate enabled and verify APRS-IS reconnects in AP+STA mode.
