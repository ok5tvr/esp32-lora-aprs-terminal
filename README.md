<h1>LoRa APRS Terminal for Waveshare ESP32 Touch LCD 3.5"</h1>

A touchscreen LoRa APRS terminal built for the Waveshare ESP32 Touch LCD 3.5" board and an external RA-02 / SX1278 433 MHz LoRa module.

The terminal can receive, decode, display and transmit APRS packets over a LoRa radio network. It combines the functions of an APRS receiver, GPS tracker, messaging terminal, weather station monitor, digipeater and receive-only APRS-IS iGate.

Created by OK5TVR

<h2>Main features</h2>
reception and display of LoRa APRS packets,
list of the latest 15 unique stations, objects and items,
decoding of standard, compressed and Mic-E positions,
APRS objects and items,
graphical APRS symbols,
distance and bearing calculation,
separate list of five weather stations,
decoding of temperature, humidity, pressure, wind and rainfall,
APRS text message transmission and reception,
message acknowledgements and retransmission,
configurable messages with or without ACK,
GPS tracker with fixed beacon intervals,
SmartBeaconing support,
standard and compressed APRS positions,
selectable tracker symbol,
instant position beacon using the BOOT button,
external NMEA GPS receiver support,
detailed GPS diagnostics,
Maidenhead grid locator calculation,
APRS digipeater with duplicate protection,
receive-only APRS-IS iGate,
touchscreen configuration,
editable callsign and default position,
Wi-Fi and APRS-IS configuration,
persistent configuration stored in ESP32 NVS,
status indicators for GPS, messages, new stations, tracker, digipeater and iGate.
<h2>Supported hardware</h2>
Waveshare ESP32 Touch LCD 3.5",
ESP32-D0WDR2-V3,
ST7796 480 × 320 display,
FT6336 capacitive touchscreen,
RA-02 / SX1278 433 MHz LoRa module,
optional NMEA GPS receiver,
optional microSD card.
<h2>Default LoRa configuration</h2>
Frequency:        433.775 MHz
Bandwidth:        125 kHz
Spreading factor: SF12
Coding rate:      4/5
Sync word:        0x12
Preamble:         8 symbols
TX power:         10 dBm
Payload CRC:      disabled
Explicit header:  enabled
<h2>RA-02 wiring</h2>
RA-02	ESP32
VCC	3.3 V
GND	GND
SCK	GPIO14
MISO	GPIO13
MOSI	GPIO26
NSS / CS	GPIO33
RESET	GPIO32
DIO0	GPIO4

<h2>Optional GPS receiver:</h2>

GPS	ESP32
TX	GPIO17
GND	GND
VCC	according to the GPS module

The GPS serial output must use a maximum logic level of 3.3 V.

<h2>APRS tracker</h2>

The integrated tracker supports:

GPS or configurable default position,
standard or compressed APRS coordinates,
fixed beacon intervals,
SmartBeaconing,
selectable APRS symbol,
manual beacon transmission using the BOOT button.

The tracker can operate independently from the periodic beacon function. A single position packet can therefore be transmitted using the BOOT button even when automatic tracking is disabled.

<h2>Digipeater</h2>

The digipeater supports:

WIDE1-1 fill-in operation,
traceable WIDE2-N,
direct callsign routing,
configurable maximum WIDE value,
duplicate packet detection,
loop prevention,
rejection of internet-originated packets,
random retransmission delay.

Example:

Received:
OK1ABC>APRS,WIDE1-1,WIDE2-1:...

<h2>Repeated:</h2>
OK1ABC>APRS,OK5TVR-17*,WIDE2-1:...
Receive-only iGate

The iGate forwards packets in one direction:

LoRa RF → Wi-Fi → APRS-IS

Packets received from APRS-IS are not transmitted back to the LoRa radio channel.

The iGate supports:

configurable Wi-Fi credentials,
APRS-IS server and port,
APRS-IS passcode,
optional server-side filter,
verified login detection,
qAO packet construction,
filtering of NOGATE, RFONLY, TCPIP and other loop-forming paths.
APRS messaging

The terminal can send and receive APRS text messages.

<h2>Supported functions include:</h2>

touchscreen recipient and message entry,
automatic message identifiers,
ACK and REJ processing,
configurable messages with or without ACK,
retransmission of unacknowledged messages,
duplicate message detection,
unread-message indicator.
GPS diagnostics

<h2>The GPS page displays:</h2>

UART data activity,
detected NMEA sentence type,
checksum statistics,
GPS fix validity,
latitude and longitude,
altitude,
speed,
course and cardinal direction,
number of satellites,
HDOP,
UTC date and time,
Maidenhead locator.

Example:

Position: 49.786333 N, 13.285000 E
Locator:  JN69PS
Speed:    42.6 km/h
Course:   064° ENE
Building the firmware

The project uses PlatformIO.
Project status

The project is under active development. Hardware testing is recommended after every firmware update, especially for radio, GPS, Wi-Fi and touchscreen functions.

<h2>Disclaimer</h2>

This project is intended for experimental amateur radio use. The operator is responsible for complying with local radio regulations, licence conditions, permitted frequencies, transmission power limits and local APRS network policies.
