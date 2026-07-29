# Architecture

The project is split by responsibility:

- `drivers`: direct hardware access
- `services`: radio/APRS logic independent of LVGL
- `ui`: screens and touch navigation
- `app`: startup, orchestration and commands
- `lib/AprsCore`: hardware-independent APRS frame functions

## SPI allocation

The classic ESP32 has two usable peripheral SPI controllers:

- **VSPI**: onboard ST7796 LCD and microSD, shared data lines with separate CS pins
- **HSPI**: external RA-02 / SX1278

Initialization order is intentional:

1. I2C, TCA9554 and display on VSPI
2. touch controller
3. LVGL
4. microSD on the already selected VSPI pins
5. RA-02 on independent HSPI

Do not move LoRa onto GPIO18/19/23; those pins belong to the onboard LCD and SD bus.

## Background receive and station history

`AppController::update()` calls `RadioService::update()` before updating the
visible screen. `RadioService` therefore receives packets continuously and is
not coupled to the LoRa status screen.

A decoded TNC2 packet is passed to `AprsCore`, which extracts the original
source callsign, symbol table/code and supported position format.
`StationStore` keeps a fixed array of 15 records with no dynamic allocation.
The newest station is index 0; duplicate callsigns are updated and moved to the
front, and the oldest unique station is discarded when the array is full.
The UI receives a read-only station view and rebuilds the scrollable station
list only when the store revision changes.


## GPS, reference position and tracker

`GpsService` reads NMEA continuously from UART2 RX GPIO17. `AppController`
chooses the current reference position: a fresh GPS fix first, otherwise the
saved default coordinates. The immutable reference snapshot is passed to both
list screens, which calculate distance and initial bearing only when their
stored entity has a position.

`TrackerService` runs after radio and GPS updates on every main-loop pass. It is
therefore independent of the Tracker UI. Its saved configuration selects GPS or
default coordinates, uncompressed or compressed APRS position encoding, and a
fixed interval or SmartBeacon schedule. The tracker submits a complete TNC2
frame through `RadioService`, so normal OE/DL framing and non-blocking SX1278 TX
handling are reused. After TX, the radio driver returns to continuous receive.

## APRS messaging

`AprsCore` parses/builds directed message and ACK/REJ frames. `MessageStore`
keeps a fixed 20-entry history, ACK queue and retransmission schedule.
`RadioService` services that queue on every loop before the tracker gets a
chance to transmit, so reception and acknowledgements remain background tasks.
