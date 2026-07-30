# Validation 1.2.5

## Purpose

Verify the corrected hardware mapping for the classic Waveshare ESP32-Touch-LCD-3.5:

- ATGM336H TX -> ESP32 GPIO4 (UART2 RX)
- RA-02 DIO0 -> ESP32 GPIO2
- GPS serial format -> 9600 baud, 8-N-1

All remaining RA-02 connections stay unchanged.

## Required hardware

- ATGM336H with 3.3 V UART output
- RA-02 / SX1278
- 10 kOhm resistor from GPIO2 to GND
- 433 MHz antenna connected before transmission

## Checks

1. Build `waveshare-esp32-release`.
2. Confirm startup log reports `UART2 listening at 9600 baud on GPIO4`.
3. Open **GPS diagnostika** and verify readable sentences beginning with `$GN` or `$GP`.
4. Confirm GPS coordinates, time, date, satellites and HDOP update.
5. Receive a LoRa APRS packet and confirm the DIO0 interrupt is processed on GPIO2.
6. Send a manual beacon and confirm the radio returns to receive mode.
7. Reboot and upload firmware repeatedly to verify GPIO2 does not block bootloader entry.
8. Confirm onboard I2S audio is not initialized.

## Expected result

GPS data are readable at 9600 baud on GPIO4, LoRa RX/TX works with DIO0 on GPIO2, and the display, SD card, tracker, Stopař and AXP2101 telemetry continue to operate normally.

## Completed host/static checks

- GPS NMEA capture regression test passed.
- Geographic calculations passed.
- APRS notification-store regression test passed.
- AXP2101 power-service test passed.
- DIGI/iGate core test passed.
- Stopař route-logger test passed.
- Compile-time checks confirm GPS RX GPIO4, LoRa DIO0 GPIO2 and GPS 9600 baud.
- Static scan found no remaining old pin constants in current configuration headers.
- No I2S/audio initialization exists in the application sources.

A complete embedded PlatformIO build could not be run in the validation container because the PlatformIO package is unavailable there. Run `pio run -e waveshare-esp32-release` before uploading.
