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

`GpsService` reads NMEA continuously from UART2 RX GPIO4. `AppController`
chooses the current reference position: a fresh GPS fix first, otherwise the
saved default coordinates. The immutable reference snapshot is passed to both
list screens, which calculate distance and initial bearing only when their
stored entity has a position.

`TrackerService` runs after radio and GPS updates on every main-loop pass. It is
therefore independent of the Tracker UI. Its saved configuration selects GPS or
default coordinates, uncompressed or compressed APRS position encoding, and a
fixed interval or SmartBeacon schedule. The tracker submits a complete TNC2
frame to the central `TxQueue`; normal OE/DL framing is applied once when the
item is enqueued. A newer scheduled tracker frame replaces an older queued one.
The non-blocking SX1278 driver receives only one selected queue item at a time
and returns to continuous receive after TX.

## Stopar route logging and SD scheduling

`TrailService` is independent from `TrackerService`. It consumes the already
decoded `GpsService::ViewState`; it never reads the GPS UART itself and never
requests radio transmission. Enabling the logger is stored in NVS through
`SettingsService`, while manual and automatic pause are runtime states.

The `AppController::update()` order is intentionally:

1. LVGL and hardware buttons
2. GPS input
3. LoRa radio receive/transmit scheduling
4. APRS tracker scheduling
5. Stopar state and SD storage
6. visible-screen refresh

The logger therefore cannot pre-empt a packet that is already waiting to be
handled by the radio service. Generated text lines are placed in a fixed
12-entry RAM queue. `TrailService::serviceStorage()` writes at most one line on
each main-loop pass and calls `flush()` only after eight lines or 15 seconds,
when the queue is empty. No dynamic queue allocation is used.

The RA-02 uses independent HSPI. The display and microSD share VSPI and their
chip-select pins, so all SD calls remain synchronous in this single-threaded
design. This avoids introducing an SPI mutex and cross-task LVGL access, but it
also means a poor microSD card can occasionally extend one loop iteration. A
quality FAT32 card is recommended. If hard real-time SD isolation is later
required, storage can be moved to a dedicated FreeRTOS task with a bounded
queue and a mutex covering the shared VSPI bus.

## Central TX queue and radio recovery

`TxQueue` is a fixed eight-entry array with no heap allocation. It stores the
fully encoded RF payload and small source metadata. The selection order is ACK,
outgoing message, DIGI, manual beacon, scheduled tracker and test. FIFO order is
preserved inside each priority. High-priority traffic may evict one lower-priority
item if all eight slots are occupied. Scheduled tracker items are coalesced.

`RadioService` first services RX and APRS parsing, then adds pending message and
DIGI frames, and finally starts at most one TX item. `MessageStore` is notified
when its frame actually starts, so ACK removal and retry timers are not advanced
merely by creating a queue entry.

`Sx1278Driver` exposes consecutive RX errors and TX timeout counters. The radio
service re-runs SX1278 initialization after an offline/error state, a TX timeout
or three consecutive RX read failures. Attempts are rate-limited and packet
counters are retained. This recovery does not reset the ESP32 or other services.

## APRS messaging

`AprsCore` parses/builds directed message and ACK/REJ frames. `MessageStore`
keeps a fixed 20-entry history, ACK queue and retransmission schedule. ACK and
outgoing-message frames enter the central queue with the two highest priorities,
so reception and acknowledgement handling remain background tasks.
