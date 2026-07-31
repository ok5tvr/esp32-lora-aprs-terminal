# Waveshare ESP32-Touch-LCD-3.5 - LoRa APRS terminal

> Version 2.7.2 extends the fully offline **Astronomie / Astronomy** page with a dynamically drawn Moon phase and current Sun altitude.
> It calculates Sun and Moon rise/set times and the current Moon phase from
> GPS/default position and the date maintained by GPS-synchronized RTC.


PlatformIO project for the **classic ESP32-D0WDR2-V3** Waveshare board with:

- 3.5 inch ST7796 display, 320 x 480
- FT6336 capacitive touch
- 16 MB Flash and 2 MB PSRAM
- onboard microSD card
- external Ai-Thinker RA-02 / SX1278 433 MHz
- LVGL 8.4 user interface

This project is **not** for the ESP32-S3 version of the board.


### Astronomy 2.7.2

- Moon phase is rendered directly by LVGL from the calculated elongation; no image files are required.
- Current Sun altitude is displayed in degrees, including negative values below the horizon.
- Dynamic astronomy values refresh every five minutes, while date or significant position changes force an immediate recalculation.

## Main menu order in version 2.7.2

1. LoRa APRS
2. Prijate stanice / Received stations
3. Zpravy / Messages
4. Meteostanice / Weather stations
5. Mapa / Map
6. Tracker
7. Stopar / Trail logger
8. DIGI / iGate
9. GPS prijimac / GPS receiver
10. Astronomie / Astronomy
11. Diagnostika / Diagnostics
12. Napajeni / Power
13. Nastaveni / Settings

## Build

Open the folder containing `platformio.ini` and run:

```powershell
pio run -e waveshare-esp32-release
pio run -e waveshare-esp32-release -t upload
pio device monitor -b 115200
```

Before the first build after replacing an older S3 project, delete the `.pio` directory.

## Hardware buses

### VSPI - onboard LCD and microSD

| Signal | GPIO |
|---|---:|
| SCK | 18 |
| MISO | 19 |
| MOSI | 23 |
| LCD CS | 5 |
| SD CS | 15 |
| LCD DC | 27 |

LCD and microSD share the data lines but have independent chip-select pins.

### HSPI - external RA-02

| RA-02 | GPIO |
|---|---:|
| SCK | 14 |
| MISO | 13 |
| MOSI | 26 |
| NSS / CS | 33 |
| RESET | 32 |
| DIO0 | 2 |
| DIO1 | not connected |
| VCC | 3.3 V only |
| GND | GND |

Connect a 433 MHz antenna before transmitting. Place 100 nF, 10 uF and 47-100 uF between 3.3 V and GND close to the RA-02 module.

GPIO2 is a boot-strapping pin. Add a 10 kOhm pulldown from GPIO2 to GND so RA-02 DIO0 remains low during reset and firmware upload. Do not enable the onboard I2S audio interface because GPIO2 and GPIO4 are reused by LoRa DIO0 and GPS RX.

### GPS

| GPS | GPIO |<br>
| TX | GPIO4 |<br>
| VCC | 3.3 V only |<br>
| GND | GND |

## Memory layout

LVGL objects are allocated preferentially in the onboard PSRAM. The display draw buffer is allocated at runtime in internal DMA-capable RAM and uses 12 display lines. This prevents large static arrays from overflowing the classic ESP32 `.dram0.bss` region. If PSRAM is unavailable, LVGL falls back to internal heap; the serial monitor reports both free internal RAM and free PSRAM at startup.

## Current functions

- splash screen
- touch menu with Up, Down, OK and Back buttons
- onboard BOOT button short press for an immediate APRS position beacon
- LoRa status with APRS/decode counters, TX queue depth, drops and recovery history
- dedicated **Diagnostika** page with a 20-point graph of five-minute background/channel RSSI measurements
- dedicated **Mapa / Map** page with microSD XYZ tiles, touch-drag panning, own position, recent Stopar trail and APRS entity symbols
- firmware name and version displayed on the **Nastaveni** page
- touchscreen LoRa profile configuration with default CZE APRS and a validated custom profile stored in NVS
- safe SX1278-only reconfiguration after the active TX and central TX queue complete
- battery-only display brightness and inactivity timeout stored in NVS
- two-stage battery backlight policy: selected brightness for 30 seconds, then 15 percent, then off at the configured timeout
- wake-only first touch/BOOT press after full blanking; activity during dimming immediately restores normal brightness
- USB-C operation fixed at full brightness with dimming and automatic blanking disabled
- main-menu local `HH:MM` clock loaded from PCF85063 RTC and synchronized from GPS UTC time
- offline **Astronomie / Astronomy** page with sunrise, sunset, daylight duration, moonrise, moonset, Moon phase, illumination and age
- fixed eight-entry central TX queue with ACK, message, DIGI, manual beacon, tracker and test priorities
- automatic RA-02-only recovery after initialization failure, TX timeout or repeated RX errors
- non-blocking receive and queued test transmission
- RSSI, SNR, frequency error and packet counters
- LoRa APRS reception with payload CRC disabled
- APRS directed-message receive/transmit with automatic ACK and retry handling
- 20-entry incoming/outgoing message history and touchscreen message composer
- OE/DL LoRa APRS header support
- background parsing of valid TNC2 APRS frames
- fixed list of the 15 most recently heard APRS stations, objects or items
- original source callsign with SSID, APRS symbol code and GPS position
- compact graphical APRS icons with overlay rendering, including the LoRa iGate `L&` symbol
- selectable Heard Stations screen; Up/Down selects an entity and OK opens its detail
- station/object/item detail with last-heard age, packet count, coordinates, RSSI, SNR and last TNC2 frame
- simple navigation to a positioned APRS entity with live distance and true bearing from GPS/default reference
- dedicated list of the five most recently heard unique APRS weather stations
- decoded temperature, humidity, pressure, wind, rain and solar-radiation data
- editable callsign and default GPS position stored in ESP32 NVS
- modal touchscreen keyboard for text and numeric settings
- distance and azimuth from the current GPS/default reference to heard stations and weather stations
- UART2 NMEA diagnostics with traffic, packet/checksum, fix, UTC, speed, course, locator and the latest complete received sentence
- main-menu GPS/default Maidenhead locator and compact GPS/message/new-station/tracker/Stopar/digipeater/iGate status indicators
- persistent APRS tracker with normal/compressed position, selectable APRS symbol and fixed/SmartBeacon scheduling
- persistent Czech/English language for the complete UI, applied immediately without restart
- independent **Stopar** GPS route logger enabled from the Tracker page
- read-only AXP2101 battery, USB-C, charger and PMIC-temperature telemetry
- permanent header summary with battery percentage, voltage and battery/charge/USB symbol
- dedicated **Napajeni** page with charger phase, configured current, target voltage and last power event
- automatic pause after 30 seconds without movement and automatic resume after movement returns
- manual pause/resume from the dedicated Stopar page
- timestamped semicolon-separated TXT logs in `/STOPAR` on microSD, with the eight newest files listed on screen
- New-N style WIDE1-1 fill-in and traceable WIDE2-N digipeater with duplicate suppression
- receive-only RF-to-APRS-IS iGate using verified login and `qAO` gating
- touchscreen WiFi, APRS-IS, DIGI mode, hop-limit and startup configuration stored in NVS
- onboard microSD initialization and capacity detection
- native unit tests for APRS framing, messages, Mic-E, objects, items, positions and weather parsing

## Configuration

Board constants and fixed packet-format constants remain in:

```text
include/app_config.h
include/lora_profile.h
include/board_pins.h
```

The operational callsign, default latitude/longitude, complete UI language, display
policy and LoRa profile are edited on the **Nastaveni / Settings** screen. Touching a numeric/text field
opens an LVGL keyboard. Values are validated and stored in ESP32 NVS, so they
survive reset and power loss. Initial defaults are:

```text
CALL:       OK5TVR-15
latitude:   49.786333
longitude:  13.285000
LoRa:       CZE APRS
frequency:  433.775 MHz
bandwidth:  125 kHz
SF / CR:    SF12 / 4/5
TX power:   10 dBm
UI language: Cestina
```

The stored callsign is used by the test transmission. The default position is
prepared for the later local GPS/beacon module and can already be changed from
the touchscreen.




## Complete UI language in version 2.6.0

The **Nastaveni / Settings** page contains a **Jazyk rozhrani / Interface language**
selector with `Cestina / Czech` and `English`. The choice remains stored under the
backward-compatible NVS key `trklang`; missing or invalid values fall back to Czech.

The selected language is loaded before hardware and services are initialized. Changing
it and saving settings rebuilds the active screen immediately, so no restart is needed.
The translation covers:

- all main-menu names and descriptions
- Settings, Tracker and DIGI/iGate editors, hints, validation and save messages
- GPS, LoRa, diagnostics, power and Trail logger states and errors
- messages, heard stations, navigation, weather and detail pages
- offline-map status, missing-tile and SD-card messages
- service-generated runtime messages shown in the UI

APRS frames, callsigns, NMEA sentences, LoRa abbreviations, units and protocol/server
replies remain unchanged because they are protocol data rather than UI text. New Trail
logger files use a localized first header line. The built-in Montserrat font currently
uses ASCII labels, therefore Czech strings are displayed without diacritics.

## LoRa module settings in version 2.4.0

The **Nastaveni** page contains a new **LoRa modul** section. The default
`CZE APRS` profile is intentionally identical to the previously hard-coded
configuration:

```text
433.775 MHz | BW 125 kHz | SF12 | CR 4/5 | TX 10 dBm
```

Selecting `Vlastni` enables these controls:

- frequency from 410.000 through 525.000 MHz
- bandwidth 62.5, 125, 250 or 500 kHz
- spreading factor SF7 through SF12
- coding rate 4/5 through 4/8
- output power 2, 5, 10, 14 or 17 dBm

The sync word remains `0x12`, preamble length remains 8, the explicit LoRa
header stays enabled and RadioLib payload CRC stays disabled. These fixed values
preserve compatibility with the terminal packet format.

Saving a changed profile does not interrupt an active transmission. The new
configuration is marked `CEKA` on the **LoRa APRS** page until the current TX and
the central TX queue are empty. The firmware then reinitializes only the SX1278,
preserves lifetime RX/TX/error counters and resumes continuous reception. GPS,
LVGL, SD logging, Stopar, map state, DIGI and iGate are not restarted.

## Main-header indicators in version 1.2.0

The main screen header is arranged as:

```text
10:18  [GPS] [message] [station] [car] [save] [digi] [L&]  74%  3,91V  [battery]
```

The clock is read from the onboard PCF85063 at startup and is corrected from
valid GPS UTC date/time. Green digits indicate a current GPS reference; white
digits indicate RTC/holdover operation. Displayed time is local CET/CEST with
the European daylight-saving transition applied automatically.

The GPS/default Maidenhead locator remains visible in the lower status area so
that the power summary does not overlap the existing service indicators.

- GPS is grey without serial traffic, orange when NMEA is active without a fix, and green with a current fix.
- The message bell displays the number of newly received APRS messages. Opening **Zpravy** clears the badge.
- The radio/station icon displays the number of newly discovered stations, objects or items. Opening **Slysene stanice** clears the badge.
- Compact badges display values 1 through 9; larger pending counts remain represented as 9 until the relevant page is opened.
- The APRS car icon `/>` is green while the tracker is active, amber while enabled but waiting for a usable position, and grey while disabled.
- The save icon is green while Stopar is recording, amber while configured but paused/waiting, red after an SD write error, and grey while disabled.
- The APRS digipeater icon `/#` is purple whenever RF digipeating is enabled.
- The LoRa iGate icon `L&` is amber while connecting and green after the APRS-IS login is verified.



## Complete APRS symbol tables in version 1.4.0

Station and weather icons now use all 94 codes from both APRS tables. The first
symbol character is no longer ignored: `/` selects the primary chart, `\` the
alternate chart, and an overlay character selects the alternate base image and
is drawn above it. This fixes collisions such as primary `/j` (jeep) versus
alternate `\j` (excavator). The images are 30 x 30 RGB565 assets stored in
Flash. Header service indicators remain recolorable alpha masks.

The tracker selection formerly called `Obecna stanice /.` has been corrected to
`Obecny bod //`, because `/.` is the red-X symbol in the APRS chart while `//`
is the red dot.

## Radio scheduling and station navigation in version 1.3.0

All RF transmissions now enter a fixed eight-entry RAM queue. No dynamic
allocation is used. The scheduler always chooses the highest-priority item and
keeps FIFO order inside the same priority:

```text
ACK -> outgoing message -> DIGI -> manual beacon -> tracker -> test
```

A newer scheduled tracker frame replaces an older queued tracker frame, so a
stale position is not transmitted after a long busy period. If the queue is
full, a newly arrived higher-priority packet may evict the newest item from the
lowest available priority. The LoRa diagnostics page displays current/max queue
depth, enqueue/replacement/drop counters and the source of the last TX.

The radio service automatically reinitializes only the SX1278 after an initial
startup failure, a TX timeout or three consecutive receive errors. Recovery is
rate-limited to one attempt per five seconds and preserves lifetime packet/error
counters. GPS, display, settings and SD logging are not restarted.

On **Prijate stanice**, Up/Down selects a row and OK opens its detail. A second
OK starts simple bearing/distance navigation when the entity has a position.
The reference is the live GPS fix when available, otherwise the configured
default position. The displayed direction is a geographic bearing; the board
has no enabled compass, so it is not a turn-by-turn heading indicator.

## Battery display power saving and RTC clock in version 2.3.0

The **Nastaveni** page contains a battery brightness slider (10-100 percent)
and an inactivity timeout selector: never, 30 seconds, 60 seconds, 2 minutes or
5 minutes. Defaults are 70 percent and 60 seconds. Both values are stored in
NVS and take effect immediately after saving.

Power-source behavior is deliberate:

- USB-C connected: backlight is forced to 100 percent and never dims or blanks
- battery only: configured brightness is used for the first 30 seconds, then the backlight is reduced to 15 percent, and it blanks at the configured inactivity timeout
- with the default 70 percent / 60 second setting the sequence is 70 % -> 15 % -> off
- PMIC unavailable: the fail-safe behavior is full brightness with no dimming or blanking

Blanking switches only the LCD backlight off. GPS, LoRa RX/TX, tracker, DIGI,
iGate, Stopar, map loading and logging continue normally. The touch controller
remains active. The first touch wakes the display and is consumed until finger
release, preventing an accidental UI action. A BOOT press while blanked also
wakes the display and is consumed, so it does not send an immediate beacon.

The backlight on GPIO25 is driven by 20 kHz 8-bit PWM. No CPU-frequency scaling
or radio duty-cycle reduction is used, so USB/full-performance operation and RF
timing are unchanged.

## RTC and GPS time service in version 2.3.0

The onboard PCF85063 is accessed on the shared I2C bus at address `0x51`.
At startup the firmware reads UTC date/time from the RTC. As soon as the
ATGM336H supplies valid NMEA date and time, GPS becomes the reference and the
RTC is updated. The normal six-hour correction interval avoids unnecessary I2C
writes; an unsuccessful initial write is retried after one minute.

The RTC stores UTC. The main-menu clock converts UTC to Czech local time using
CET/CEST rules, including the last-Sunday transitions in March and October.
If neither RTC nor GPS contains a valid date/time, the header displays `--:--`.
Detailed behavior is described in `docs/TIME_SERVICE.md`.

## Offline map in version 2.1.0

The **Offline mapa** screen reads standard Web Mercator XYZ tiles from the
microSD card. To keep the embedded renderer small and deterministic, each tile
is stored as a raw 256 x 256 little-endian RGB565 file:

```text
/MAP/<zoom>/<x>/<y>.rgb
```

The map initially follows the current GPS fix, or the configured default
position when a fix is unavailable. It displays a blue own-position marker, up
to 15 positioned APRS stations/objects/items with their full APRS symbols,
emergency highlighting and an orange polyline containing the latest 64 Stopar
points. Dragging the map with one finger enters manual mode (`MAN`). The already
rendered map and overlays follow the finger immediately, while the new tiles are
requested only after release. Up/Down changes zoom from 3 through 18 and OK
returns to GPS/default following and recenters the map.

Tile reads are incremental: at most eight rows of one tile are read in a main
loop pass, after GPS, LoRa RX/TX, tracker and Stopar processing. The 480 x 202
RGB565 framebuffer uses 193920 bytes and is allocated in PSRAM.
Missing tiles are shown as a grey checkerboard instead of blocking the terminal.

Use `tools/convert_map_tiles.py` to convert an existing PNG/JPEG/WebP XYZ tile
tree. Detailed preparation and licensing notes are in `docs/OFFLINE_MAP.md`.

## Firmware and RF-channel diagnostics in version 1.5.0

The **Nastaveni** page shows the firmware name and `AppConfig::FIRMWARE_VERSION`.
The separate **Diagnostika** page monitors instantaneous SX1278 channel RSSI.
After a short startup delay, one point is stored every five minutes. Each point
is the average of eight register reads spaced 25 ms apart, while the strongest
read is retained as the peak. The fixed RAM history contains 20 points and is
cleared by a device restart.

Measurements start only while the radio is initialized in continuous RX and
the central TX queue is empty. If TX starts or traffic is queued during the
short measurement burst, the burst is cancelled and retried later. Reading the
RSSI register does not intentionally leave RX mode. The result should be
interpreted as channel background/activity at the sampling moment: a legitimate
LoRa packet or another signal can raise the measured value, so this is not a
calibrated spectrum analyzer.

## AXP2101 power management in version 1.2.0

The onboard AXP2101 is read through the shared I2C bus on GPIO21/GPIO22. The
firmware enables only the measurement channels required for telemetry and does
not change the charger current, target voltage or PMIC output rails.

The permanent header summary displays battery percentage and battery voltage.
Its final symbol and colour have the following meaning:

- battery symbol: normal operation from the accumulator
- lightning symbol, green: charging is active
- USB symbol, blue: USB-C is connected and active charging is not in progress
- battery symbol, red: battery is critical (10 percent or 3.40 V by default)

The **Napajeni** page displays:

- battery presence, percentage and voltage
- operating state (charging, discharging, USB-C supply or standby)
- charger phase (pre-charge, constant current, constant voltage, done or stopped)
- configured charge-current setting and charge-target voltage
- USB-C/VBUS presence, VBUS validity and measured VBUS voltage
- system voltage and internal PMIC die temperature
- the most recent detected transition, such as USB connection/disconnection,
  battery connection/disconnection, charge start/end or critical battery

Power values are polled every two seconds after GPS, LoRa, tracker and Stopar
processing. The operation is short, read-only I2C traffic and does not perform
SD writes. Thresholds and polling interval are defined in `include/app_config.h`.

The PMIC temperature is the internal AXP2101 die temperature, not the Li-Pol
cell temperature. The displayed charge current is the configured charger
setting, not a live measurement of battery current.


## Stopar route logger in version 1.1.0

Stopar is enabled persistently on the **Tracker** page, but it has its own
main-menu page for status, manual pause/resume and browsing the eight newest
TXT logs. Recording starts after a valid GPS position and UTC date/time are
available. A new file is created for every recording session:

```text
/STOPAR/YYYYMMDD_HHMMSS.txt
```

Every data row is plain text and uses semicolon-separated fields:

```text
UTC;latitude;longitude;altitude_m;speed_kmh;course_deg;satellites;hdop;state
```

The file also contains comment lines for `START`, `STOP`, `AUTO_PAUSE`,
`AUTO_RESUME`, `MANUAL_PAUSE` and `MANUAL_RESUME`. Time is stored as GPS UTC,
so the files are independent of local daylight-saving changes.

The default sampling interval is 5 seconds. A point is retained after at
least 3 metres of movement, or after 30 seconds even when the movement
threshold was not reached. Autopause activates after 30 seconds without
movement. Recording resumes at 2 km/h or after an 8 metre displacement.
These values are constants in `include/app_config.h`.

Stopar is deliberately serviced after GPS, LoRa receive and APRS tracker work.
It queues text rows in RAM, writes at most one queued row per loop and batches
`flush()` calls. LoRa uses independent HSPI, while LCD and microSD share VSPI.
With a healthy FAT32 microSD card the logger should not noticeably affect
normal reception or tracker scheduling. SD access is nevertheless synchronous,
so a slow, damaged or heavily fragmented card can still cause an occasional
short UI or loop delay. See `docs/TRAIL_LOGGER.md` for details.


## DIGI / iGate in version 1.0.2

The **DIGI / iGate** page configures two independent services:

- a radio digipeater using `WIDE1-1`, `WIDE2-N` or both modes
- a receive-only iGate that sends eligible RF packets to APRS-IS

The digipeater processes only the first unused path element. A `WIDE1-1`
request is replaced by the configured callsign with the used marker. A
`WIDE2-N` request is traced with the configured callsign and decremented when
additional hops remain. The local maximum is limited to one or two hops. A
30-second path-independent duplicate cache and a short randomized delay reduce
repeat loops and simultaneous retransmissions.

The iGate operates only in the RF-to-Internet direction. It requires WiFi, an
APRS-IS server and a valid passcode for the callsign configured on the main
**Nastaveni** page. Packets containing `NOGATE`, `RFONLY`, `TCPIP`, `TCPXX`, an
existing q construct or a generic query are not gated. Eligible third-party
frames are unwrapped and forwarded with `qAO,<IGATECALL>`. Packets waiting
more than 30 seconds during a network outage are dropped instead of being sent
later as a stale burst. Internet-to-RF gating is intentionally not included
because it requires recent-heard-station
tracking, strict message-only selection and third-party RF encapsulation.

WiFi credentials and the APRS-IS passcode are stored in ordinary ESP32 NVS.
They are not encrypted unless Flash encryption is enabled separately for the
device. Changing the operating callsign also requires changing the APRS-IS
passcode to one valid for the new callsign.

See `docs/DIGI_IGATE.md` for path examples, filtering and limitations.

## Touch orientation

The default landscape transformation is:

```text
screen X = raw Y
screen Y = 319 - raw X
```

If the touch direction is mirrored on a particular panel revision, adjust only these constants in `include/board_pins.h`:

```cpp
TOUCH_SWAP_XY
TOUCH_MIRROR_X
TOUCH_MIRROR_Y
```

## Project structure

```text
boards/       custom PlatformIO board definition
include/      central configuration
src/app/      application controller and menu model
src/drivers/  display, touch, LVGL, SD and SX1278
src/services/ APRS/radio application services
src/ui/       screens and reusable UI components
lib/AprsCore/ hardware-independent APRS frame codec
test/         native unit tests
docs/         wiring and architecture notes
```

## Reset-safe screen navigation

Version 0.4.2 queues touch navigation and changes screens only after the LVGL
event callback has returned. This avoids deleting the active button while LVGL
is still dispatching its click event and prevents accidental click-through into
the LoRa screen.

The serial monitor also reports the previous reset reason. A `brownout` result
indicates a 3.3 V power problem; `panic/exception` points to a software crash.

## Version 0.4.3 fixes

- LoRa status labels use safe floating-point formatting.
- Opening the LoRa screen no longer crashes inside `lv_printf.c`.
- Radio interrupt callbacks are switched without removing an uninstalled handler.


## APRS messages in version 0.9.0

The **Zpravy** menu remains active while any other screen is displayed. Incoming
TNC2 message frames addressed to the callsign stored in **Nastaveni** are added
to a fixed 20-entry RAM history. Messages addressed to `ALL`, `QST` or `CQ` are
also shown, but are not acknowledged.

Pressing **OK** on the message screen opens a two-step touch editor. The first
step edits the recipient (up to nine characters including SSID); the second
edits up to 67 printable ASCII characters. APRS-reserved message characters
`|`, `~` and `{` are rejected.

Outgoing messages use a three-digit message identifier:

```text
OK5TVR-15>APRS::OK1ABC-7 :Ahoj{003
```

The nine-character addressee field is padded with spaces. An incoming message
with an identifier is acknowledged automatically:

```text
OK5TVR-15>APRS::OK1ABC-7 :ack123
```

The history displays `CEKA ACK`, `ACK`, `REJ` or `CHYBA`. An unacknowledged
message is retransmitted with progressively longer delays, up to five total
transmissions. A repeated incoming message with the same source and identifier
does not create another row, but another ACK is scheduled. Message history is
currently held in RAM and is cleared after reset or power loss.

## Heard station list

Reception is serviced from the application main loop regardless of the visible
screen. A packet can therefore update the station list while the menu, GPS,
settings or LoRa status screen is open.

The list contains at most 15 APRS entities. Normal stations are keyed by the
original source callsign. Objects and items are keyed separately by their
case-sensitive APRS name, so several objects from one station do not overwrite
each other. Every object/item also retains the original transmitting callsign as
its owner. A repeated entity is updated and moved to the top; the oldest entry
is removed when a sixteenth entity is heard. One level of third-party
encapsulation (`}`) is recognized and uses the inner source.

For each station the screen shows:

```text
OK5TVR-15       />
49.73667 N   13.38500 E
```

The first symbol character is the APRS symbol table (`/` or `\` in the
normal case) and the second is the APRS symbol code. Standard uncompressed and compressed positions using the `!`, `=`, `/` and
`@` data type identifiers are decoded. Mic-E positions are decoded directly
from the six-character destination and the binary-safe information field.
Live APRS objects (`;`) and items (`)`) support both uncompressed and compressed
positions. Killed objects/items are removed from the visible list. A station
received without a position is still listed; if it had a position earlier, its
last known position is retained.

The station history is currently held in RAM and is cleared by reset or power
off.


## APRS entities in version 0.6.0

Normal stations are displayed by callsign. Objects and items include both their
name and the original owner/source callsign:

```text
OK5TVR-15                         />
49.73667 N   13.38500 E

OBJ:LEADER  <OK1ABC>              />
49.05833 N   72.02917 W

ITEM:AID#2  <OK2XYZ>              /A
49.05833 N   72.02917 W
```

LoRa payload CRC remains explicitly disabled in
`src/drivers/sx1278_driver.cpp` with `radio_.setCRC(false)`.

## Weather stations in version 0.7.0

Weather decoding runs in the same background receive path as the normal APRS
entity list. The visible screen therefore does not need to be open while a
weather packet is received.

The weather list is keyed by the original source callsign, including SSID. It
contains five unique stations. Hearing the same station updates its known
values and moves it to the top. A sixth unique station removes the oldest
record.

Decoded fields include:

```text
txxx       temperature in degrees Fahrenheit -> displayed in degrees Celsius
hxx        relative humidity, where 00 means 100 percent
bxxxxx     barometric pressure in tenths of hPa
gxxx       five-minute wind gust in mph -> km/h
rxxx       rain in the last hour, hundredths of an inch -> mm
pxxx       rain in the last 24 hours, hundredths of an inch -> mm
Pxxx       rain since midnight, hundredths of an inch -> mm
Lxxx/lxxx  solar radiation in W/m2
cddd/sddd  positionless-weather direction and speed
DDD/SSS    complete-weather direction and speed data extension
```

When a later packet omits one of the measurements, the last known value for
that station is retained. A new packet position and APRS symbol update the
stored station location.

## Touch settings

The settings screen contains three editable fields:

```text
CALL
Sirka   (-90 to 90 decimal degrees)
Delka   (-180 to 180 decimal degrees)
```

Touching CALL opens an uppercase keyboard limited to letters, digits and `-`.
Touching a coordinate opens the numeric keyboard. `Ulozit` or the bottom `OK`
button validates the fields and writes them to NVS.

## GPS, relative position and tracker in version 0.8.0

A generic NMEA GPS receiver can be connected receive-only to UART2:

| GPS module | Waveshare ESP32 |
|---|---:|
| TX | GPIO4 |
| GND | GND |
| VCC | use the voltage specified for the particular GPS breakout |
| RX | not connected |

The signal presented to ESP32 GPIO4 must use 3.3 V logic. The default serial
format is 9600 baud, 8-N-1. The firmware regards the receiver as present after
receiving valid NMEA sentences and regards a position fix as usable while it is
not older than five seconds.

The station and weather screens calculate great-circle distance and the initial
bearing from a reference position. A fresh GPS fix has priority. If no fresh
fix is available, the latitude and longitude saved on the **Nastaveni** screen
are used. The header identifies the source as `Ref: GPS` or `Ref: DEF`.

The **Tracker** menu stores these settings in NVS:

```text
Tracker       enabled / disabled
Source        GPS / default configured position
Format        normal uncompressed / Base-91 compressed
APRS symbol   selected from a touch dropdown
Scheduling    fixed interval / SmartBeacon
Interval      30, 60, 120, 180, 300, 600, 900, 1800 or 3600 seconds
```

The symbol list contains car `/>`, pedestrian `/[`, bicycle `/b`, motorcycle
`/<`, fixed QTH `/-`, boat `/s`, aircraft `/^`, balloon `/O`, weather station
`/_`, generic point `//` and LoRa iGate `L&`. The selection is stored in NVS
and is used for both normal and compressed tracker packets.

GPS-source tracking can be enabled only after a valid NMEA receiver has been
detected. Transmission then waits for a fresh position fix. A tracker using the
saved default position can work without a GPS receiver, but uses fixed interval
beaconing because a static position has no speed or course for SmartBeacon.

SmartBeacon adjusts the periodic interval according to speed and can send an
early corner beacon after a sufficient course change. Normal position packets
include `ddd/sss` course and speed when fresh GPS movement data are available.
Compressed packets use the APRS Base-91 position and compressed course/speed
fields. The tracker uses the stored callsign and the APRS symbol selected in the Tracker menu.

All services remain active independently of the visible screen: LoRa reception,
station/weather collection, GPS parsing and tracker scheduling continue while
any menu page is open.

## GPS diagnostics in 0.9.2

The GPS page now reports UART/NMEA traffic, complete and checksum-valid packet ages,
last sentence type, fix quality, position, altitude, speed, course, UTC time/date and a
six-character Maidenhead locator. The main menu header shows `GPS <locator>` for a
current fix or `DEF <locator>` for the configured fallback position. See
`docs/GPS_DIAGNOSTICS.md`.

## DIGI / iGate quick selection

- Receive-only iGate: DIGI OFF, RX iGate ON.
- Digipeater only: DIGI ON, RX iGate OFF.
- Both functions: both switches ON.
- Passive receiver only: both switches OFF.


## Uvodni obrazovka

Uvodni obrazovka zobrazuje autora projektu: `Vytvoril: OK5TVR`.

## Onboard BOOT beacon button

A short press of the physical **BOOT** button sends one position packet using
the saved tracker source, format and APRS symbol. The periodic tracker does not
have to be enabled. GPS mode waits for a fresh fix; Default mode uses the saved
coordinates. See `docs/HARDWARE_BUTTONS.md`. Do not hold BOOT during reset or
power-up because GPIO0 also selects the ESP32 download mode.


## APRS parser 1.4.0

Parser rozlisuje stanice, objekty a polozky, normalni/komprimovanou/Mic-E polohu, klasickou telemetrii T#, PHG, emergency a frekvencni udaje v komentari. Seznam meteostanic podporuje vyber a otevreni samostatneho detailu tlacitkem OK.
