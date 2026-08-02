# LoRa APRS Terminal User Manual

Welcome to the English user manual for the **Waveshare ESP32 LoRa APRS Terminal**.

The firmware turns the classic Waveshare ESP32-Touch-LCD-3.5 board into a standalone 433 MHz LoRa APRS terminal with a touch interface, GPS support, APRS messaging, tracker, digipeater, receive-only APRS-IS iGate, offline maps, route logging, power monitoring and browser-based OTA updates.

> [!IMPORTANT]
> This project is for the **classic ESP32-D0WDR2-V3 Waveshare ESP32-Touch-LCD-3.5 board** with 16 MB Flash and 2 MB PSRAM. It is not compatible with the ESP32-S3 version without substantial changes.

## Current release

This manual describes firmware **v2.7.7**.

Version 2.7.7 fixes the startup stack overflow found in v2.7.6 while preserving APRS route analysis, persistent battery history and Web OTA support.

## Main capabilities

- LoRa APRS reception and transmission on the Czech 433.775 MHz profile
- Configurable custom LoRa profile
- APRS station, object, item and weather decoding
- DIRECT / VIA DIGI route analysis
- APRS text messaging with ACK and retry handling
- GPS tracker with fixed interval or SmartBeacon scheduling
- WIDE1/WIDE2 digipeater
- Receive-only RF-to-APRS-IS iGate
- Offline map from microSD tiles
- Independent GPS trail logger
- Persistent battery history and power telemetry
- Czech and English interface
- Local Web OTA update at `192.168.4.1`

## Start here

1. [Getting Started](Getting-Started)
2. [Hardware and Wiring](Hardware-and-Wiring)
3. [Installation and Build](Installation-and-Build)
4. [Settings Reference](Settings-Reference)
5. [User Interface](User-Interface)

## APRS functions

- [APRS Reception and Route Analysis](APRS-Reception-and-Route-Analysis)
- [APRS Messages](APRS-Messages)
- [LoRa Radio and Diagnostics](LoRa-Radio-and-Diagnostics)
- [GPS and Time](GPS-and-Time)
- [Weather and Astronomy](Weather-and-Astronomy)
- [Tracker and Trail Logger](Tracker-and-Trail-Logger)
- [DIGI and iGate](DIGI-and-iGate)
- [Offline Map](Offline-Map)

## Maintenance

- [Power Management](Power-Management)
- [Web OTA Update](Web-OTA-Update)
- [Troubleshooting](Troubleshooting)
- [Development and Architecture](Development-and-Architecture)
- [Release History](Release-History)

## Safety notes

- Always connect a suitable **433 MHz antenna before transmitting**.
- Power the RA-02 from **3.3 V only**.
- Add local decoupling capacitors near the RA-02.
- Do not hold the BOOT button while powering or resetting the ESP32.
- Use only a protected Li-Pol/Li-ion battery whose charging specification is known.
- Do not remove the microSD card while the Trail logger is recording.
