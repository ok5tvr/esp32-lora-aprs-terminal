# GPS and Time

## GPS input

The firmware reads NMEA data on UART2 RX GPIO4 at 9600 baud, 8-N-1.

The GPS receiver page displays:

- UART number, GPIO and baud rate
- latest NMEA sentence type
- age of the newest complete sentence
- age of the newest checksum-valid sentence
- latitude, longitude and altitude
- Maidenhead locator
- speed, course and cardinal direction
- satellites and HDOP
- UTC date and time
- latest complete NMEA sentence

## Detection states

1. service started, no bytes
2. bytes received, no complete line
3. complete line received, no recent valid checksum
4. valid NMEA packets received, with or without position fix

A current position fix expires after a short timeout when valid position data stops arriving.

## Reference position

Services use this priority:

1. current GPS fix
2. saved default latitude and longitude

The active source appears in the main header:

```text
GPS JN69PS
DEF JN69PS
```

The same reference is used for maps, station navigation and astronomy. The tracker may independently be configured to require GPS or to use the default position.

## RTC synchronization

The onboard PCF85063 RTC is read during startup and stores UTC.

Valid GPS UTC date and time periodically synchronize the system clock and RTC. Local display time is converted to CET/CEST using European daylight-saving rules.

Clock colors:

- green: current GPS time is the active reference
- white: RTC or holdover time
- `--:--`: no valid time source

The astronomy page needs a valid local date and a reference position, but it does not require internet access.
