# Adding a NMEA GPS receiver

Version 0.8.0 uses UART2 in receive-only mode. The GPS module sends NMEA data
to the ESP32; the ESP32 does not need to transmit commands back to the module.

| GPS signal | Waveshare ESP32-Touch-LCD-3.5 |
|---|---:|
| TX | GPIO17 (ESP32 UART2 RX) |
| RX | not connected |
| GND | GND |
| VCC | according to the GPS breakout specification |

The UART signal connected to GPIO17 must be 3.3 V logic. Do not connect a raw
5 V serial output directly to the ESP32 input. The firmware default is 9600
baud, 8 data bits, no parity and one stop bit.

GPIO17 is independent of UART0, so firmware upload and the USB serial monitor
remain available. RA-02 DIO0 uses GPIO4 in the current wiring; GPIO17 is no
longer assigned to the radio.

The GPS service detects hardware from valid NMEA checksums. The tracker can be
saved with GPS as its source only after the receiver has been detected. It will
then wait for a fresh position fix before sending. The default-position source
can transmit without GPS and uses coordinates saved on the Settings screen.
