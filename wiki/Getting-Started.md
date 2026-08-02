# Getting Started

This page describes the shortest path from assembled hardware to receiving LoRa APRS packets.

## 1. Required hardware

- Waveshare ESP32-Touch-LCD-3.5, classic ESP32 version
- Ai-Thinker RA-02 / SX1278 433 MHz module
- 433 MHz antenna
- Optional ATGM336H or other NMEA GPS receiver
- Optional FAT32 microSD card
- USB-C cable and a stable 5 V supply
- Optional protected 1-cell Li-Pol/Li-ion battery

See [Hardware and Wiring](Hardware-and-Wiring) before applying power.

## 2. Build and upload

Open the project folder containing `platformio.ini` and run:

```powershell
pio run -e waveshare-esp32-release
pio run -e waveshare-esp32-release -t upload
pio device monitor -b 115200
```

For the first installation of an OTA-capable version, upload the complete project through USB. See [Installation and Build](Installation-and-Build).

## 3. First boot

A normal serial startup contains lines similar to:

```text
[I][APP] LoRa APRS Terminal v2.7.7
[I][DISPLAY] Ready: 480 x 320 on VSPI
[I][POWER] AXP2101 ready
[I][TOUCH] Ready
[I][SD] SDHC card ready
[I][GPS] UART2 listening at 9600 baud on GPIO4
```

If the device repeatedly restarts or reports a panic, go to [Troubleshooting](Troubleshooting).

## 4. Basic configuration

Open **Settings** and configure:

- operating callsign and SSID
- default latitude and longitude
- interface language
- display brightness and timeout
- LoRa profile

For normal Czech LoRa APRS operation, keep the preset:

```text
433.775 MHz | BW 125 kHz | SF12 | CR 4/5 | TX 10 dBm
```

Press **Save**. Settings are stored in ESP32 NVS and survive reset and power loss.

## 5. Verify reception

Open **LoRa APRS**. Confirm that:

- the radio is online
- RX mode is active
- the selected frequency and modulation are correct
- packet and decode counters increase when traffic is present

Open **Received stations** to inspect decoded stations, objects and items.

## 6. Optional GPS

Connect a 3.3 V NMEA output to GPIO4. The default input is UART2 at 9600 baud, 8-N-1.

Open **GPS receiver**. A healthy receiver progresses through these states:

1. no traffic
2. bytes received
3. complete NMEA sentences received
4. checksum-valid NMEA packets
5. current position fix

## 7. Test transmission

The onboard BOOT button can send a one-shot position beacon:

- short press: request one manual beacon
- if the display is fully off, the first press only wakes it
- do not hold BOOT during reset or power-on

A manual beacon uses the saved tracker source, packet format and APRS symbol. Ensure that transmitting is legal at your location and that an antenna is connected.
