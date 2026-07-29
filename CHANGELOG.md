## 1.0.5

- Added a car APRS icon (`/>`) to the main header for tracker state.
- The tracker icon is green while scheduled tracking is active, amber while enabled but waiting for a usable position, and grey when disabled.
- Added a digipeater APRS icon (`/#`) that turns purple whenever RF digipeating is enabled.
- Added the LoRa iGate APRS icon (`L&`) using the same gateway asset and `L` overlay as station/object rendering.
- The iGate icon is green after a verified APRS-IS login, amber while enabled but not verified, and grey when disabled.
- Compacted all header indicator boxes to 30 px and repositioned them between `LoRa` and the Maidenhead locator.
- Retained message and new-station badges, the BOOT manual beacon, and `radio_.setCRC(false)`.

## 1.0.4

- Shortened the main-menu header title from `LoRa APRS terminal` to `LoRa`.
- Added three compact header indicators between the title and Maidenhead locator.
- GPS indicator uses grey for no traffic, orange for NMEA activity without a fix and green for a valid fix.
- Incoming APRS messages light the notification icon and increment an unread badge.
- Newly discovered stations, objects and items light the RF icon and increment a new-entity badge.
- Message notifications clear when the Messages page is opened; station notifications clear when Heard Stations is opened.
- Repeated copies of the same identified APRS message and repeated packets from an already-known station do not increment the badges.
- Added monotonic event counters to the fixed-size message and station stores and a native regression test.
- Retained the BOOT manual beacon and LoRa payload CRC disabled with `radio_.setCRC(false)`.

## 1.0.3

- Added debounced polling of the onboard BOOT button on GPIO0.
- A short BOOT press requests an immediate APRS position beacon.
- Manual beaconing works while periodic tracking is disabled and uses the saved tracker source, packet format and APRS symbol.
- GPS-source manual requests wait up to 15 seconds for a valid fix; default-position requests can transmit immediately.
- A busy radio delays the request instead of dropping it, while messages and digipeater traffic retain their existing scheduler priority.
- Manual transmission resets the normal tracker interval to avoid an immediate duplicate scheduled beacon.
- Added main-menu and tracker-screen feedback for BOOT beacon requests.
- RESET and PWR retain their original hardware functions.
- Retained LoRa payload CRC disabled with `radio_.setCRC(false)`.

# Changelog

## 1.0.2

- na uvodni obrazovku doplnen text `Vytvoril: OK5TVR`
- indikator spousteni byl posunut nize, aby se texty neprekryvaly

## 1.0.1

- opraveno prekryvani bileho nazvu a sede napovedy na strance DIGI / iGate
- svisle odsazeni radku je nyni explicitni a seda napoveda je posunuta o 4 px vyse
- popisky rozlisuji Digipeater RF -> RF a receive-only iGate RF -> APRS-IS
- doplnena kratka napoveda pro samostatne zapnuti jednotlivych sluzeb

## 1.0.0

- Added a dedicated touchscreen **DIGI / iGate** configuration and diagnostics page.
- Added persistent NVS enable switches for the digipeater and receive-only APRS-IS iGate.
- Added New-N style WIDE1-1 fill-in and traceable WIDE2-N operation with a configurable local maximum of one or two hops.
- Repeats only the first unused path component, marks the local callsign as used and decrements WIDE2-N when another hop remains.
- Added directed digipeating through an unused local callsign path and rejection of packets already repeated by this station.
- Added a 30-second path-independent duplicate cache and a randomized 120-420 ms retransmission delay.
- Added WiFi STA and APRS-IS configuration for server, port, passcode and optional server filter.
- Added verified APRS-IS login and RF-to-IS gating with the receive-only `qAO` construct.
- Honors `NOGATE`, `RFONLY`, `TCPIP`, `TCPXX`, existing q constructs and generic APRS queries.
- Unwraps eligible third-party packets before RF-to-IS gating and rejects Internet-origin third-party loops.
- Keeps Internet-to-RF gating disabled; this release is deliberately a receive-only iGate.
- Added fixed-capacity DIGI/iGate queues, counters and host-side tests without dynamic packet allocation.
- Discards APRS-IS queue entries older than 30 seconds after a network outage instead of uploading a stale burst.
- Reserves APRS-IS line space for the appended qAO path and enforces the 512-byte CR/LF line limit.
- Validates the RF/APRS-IS login callsign before allowing iGate startup.
- Retained the deployed LoRa profile with `radio_.setCRC(false)`.

## 0.9.3

- Added a touch dropdown for selecting the APRS symbol transmitted by the tracker.
- Added common choices for car, pedestrian, bicycle, motorcycle, QTH, boat, aircraft, balloon, weather station, generic station and LoRa iGate.
- Stored the selected tracker symbol persistently in ESP32 NVS.
- Applied the selected symbol to both normal and Base-91 compressed APRS positions.
- Added motorcycle rendering to the compact APRS icon mapper.
- Retained GPS diagnostics, messaging, SmartBeacon and `radio_.setCRC(false)`.

## 0.9.2

- Added live GPS/NMEA diagnostics: UART traffic, complete sentence and valid-checksum detection.
- Added last NMEA sentence type, packet ages, sentence counter and characters-per-second display.
- Added GPS UTC date/time, position, altitude, speed, course and cardinal direction display.
- Added six-character Maidenhead locator calculation, for example `JN69PS`.
- Added `GPS/DEF <locator>` status to the main LoRa APRS terminal header.
- Retained all APRS messages, tracker, weather, object/item and icon functions from 0.9.1.

## 0.9.1

- replace the two-character APRS symbol text in station/object/item and weather lists with compact graphical icons
- add 24 x 24 alpha-only LVGL assets for common mobile, fixed, weather, digi, gateway, balloon, aircraft, boat, bicycle and repeater symbols
- render APRS overlays on top of the alternate-table base icon
- explicitly support the LoRa iGate convention `L&` as a gateway diamond with an `L` overlay
- preserve the original two-character APRS code as a fallback for unsupported symbols
- keep image assets linked in firmware Flash and recolor them at runtime
- retain LoRa payload CRC disabled with `radio_.setCRC(false)`

## 0.9.0

- add APRS directed-message parsing and encoding with the fixed nine-character addressee field
- add a Messages screen with a 20-entry incoming/outgoing RAM history
- add a two-step touchscreen composer for recipient and message text
- generate three-digit message identifiers and request acknowledgements for outgoing messages
- automatically transmit `ack` responses for directed messages addressed to the configured callsign
- recognize incoming `ack` and `rej` responses and update delivery state
- retry unacknowledged messages up to five times while keeping LoRa reception active in the background
- merge duplicate identified incoming messages while acknowledging repeated copies
- accept group messages addressed to `ALL`, `QST` and `CQ` without acknowledging them
- support one level of third-party encapsulation for APRS messages
- retain LoRa payload CRC disabled with `radio_.setCRC(false)`

## 0.8.1

- fixed build errors caused by references to unavailable LVGL built-in fonts
- replaced Montserrat 12/13/20 usages with enabled Montserrat 14/18 fonts
- added explicit LVGL style-selector casts to silence GCC 14 enum warnings

## 0.8.0

- add distance and initial-bearing calculation from the current reference position to heard APRS entities and weather stations
- prefer a fresh GPS fix as the reference position and fall back to the configured default latitude/longitude
- add a generic NMEA GPS service on UART2 RX GPIO17 at 9600 baud using TinyGPSPlus
- add a live GPS status screen with receiver detection, fix, position, altitude, speed, course, satellites and HDOP
- add a persistent APRS tracker screen and NVS configuration
- select GPS or default coordinates as the tracker position source
- select normal uncompressed or Base-91 compressed APRS position reports
- select fixed-interval beaconing or GPS-based SmartBeacon scheduling
- prevent SmartBeacon selection with the static default position
- require a detected GPS receiver before enabling GPS-source tracking, then wait for a valid fix before transmitting
- continue LoRa reception, APRS parsing, GPS reading and tracker scheduling in the background on every screen
- re-arm the tracker whenever saved tracker settings change
- add APRS position-frame encoder tests and host-side distance/bearing tests
- retain LoRa payload CRC disabled with `radio_.setCRC(false)`

## 0.7.0

- add a dedicated APRS weather-station screen with five unique source callsigns
- move an already-known weather station to the top and overwrite the oldest entry when the sixth station is heard
- decode common APRS weather fields: temperature, humidity, pressure, wind direction, wind speed, gust, rainfall and solar radiation
- support complete weather reports with `ddd/sss`, positionless weather fields `cddd`/`sddd`, and weather data following station, object or item positions
- convert displayed values to degrees Celsius, percent, hPa, km/h and millimetres
- keep LoRa reception, APRS decoding and both station stores active on every screen
- add persistent NVS settings for callsign and default latitude/longitude
- use the saved callsign for the test transmission
- add a modal LVGL touch keyboard for callsign and numeric coordinate editing
- retain `radio_.setCRC(false)`; no `setCRC(true)` call is present

## 0.6.0

- add binary-safe Mic-E decoding from the TNC2 destination and information fields
- preserve non-printable Mic-E bytes for parsing while keeping the UI text printable
- decode Mic-E latitude, longitude and the symbol table/code
- decode live and killed APRS objects with fixed nine-character names
- decode live and killed APRS items with variable three-to-nine-character names
- store objects and items as independent entities instead of assigning their position to the transmitting station
- preserve the original transmitting callsign as the object/item owner
- remove killed objects/items from the visible list
- keep object/item names case-sensitive as required by APRS
- confirm and retain `radio_.setCRC(false)` for LoRa APRS compatibility


## 0.5.0

- add a fixed-memory list of the 15 most recently heard unique APRS stations
- keep the original source callsign, including SSID, from the TNC2 frame
- decode APRS symbol table/code as `/x`, `\x` or the received overlay code
- decode standard uncompressed and compressed GPS positions for `!`, `=`, `/` and `@` packets
- preserve a station's last known position when a later status packet has no position
- move an already-known station to the top when it is heard again
- replace the oldest station when a sixteenth unique callsign is heard
- add a touch-scrollable Heard Stations screen
- keep LoRa reception and station collection active in the background on every screen
- use the original inner source callsign for one-level third-party (`}`) frames
- disable LoRa payload CRC for the deployed LoRa APRS profile
- replace the obsolete CRC counter with a general RX error counter

## 0.4.3

- fixed LoadProhibited crash in the LoRa screen caused by LVGL float formatting
- use the standard C formatter for LVGL formatted labels
- format LoRa parameters and signal values into bounded local buffers
- guard LoRa label pointers before periodic updates
- avoid clearing a RadioLib interrupt callback before one has been installed
- remove the harmless GPIO ISR service warning during the first RX startup

## 0.4.2

- defer all navigation until after the LVGL event callback has returned
- prevent deletion of the active navigation button from its own callback
- add a short navigation lock to prevent click-through into a newly created screen
- use GPIO4 for RA-02 DIO0; onboard audio must remain disabled
- log the previous ESP32 reset reason and free memory at startup

## 0.4.1

- moved LVGL dynamic allocations to PSRAM with internal-RAM fallback
- replaced the large static LVGL draw buffer with a runtime DMA-capable buffer
- reduced the draw buffer from 30 to 12 lines
- removed the 80 kB static LVGL heap from `.dram0.bss`
- added startup memory diagnostics
- fixed classic ESP32 linker overflow in `dram0_0_seg`

## 0.4.0

- changed target from ESP32-S3 to classic ESP32-D0WDR2-V3
- added custom PlatformIO board definition for 16 MB Flash and 2 MB PSRAM
- corrected ST7796, FT6336, TCA9554 and I2C pin mapping
- placed LCD + microSD on VSPI and RA-02 on independent HSPI
- added onboard microSD initialization
- corrected touch transformation for landscape rotation 1
- reduced LVGL internal memory use for the classic ESP32

## 0.3.0

- initial modular project with LVGL and RA-02 support
