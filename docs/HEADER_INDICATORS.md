# Main header indicators

Firmware 2.3.0 replaces the compact `LoRa` text in the main-menu header with local `HH:MM` time and keeps the seven service status groups before the power summary.

## GPS

- grey: no recent UART traffic
- dark orange: bytes are arriving but no complete current NMEA status
- orange: NMEA receiver detected but no current position fix
- green: current GPS fix

## APRS message

The bell turns orange and receives a red badge when a new APRS text message addressed to the local callsign, ALL, QST or CQ is inserted. Repeated copies with the same source and message ID are acknowledged as before but do not increase the badge. Opening the Messages page marks pending notifications as seen.

## Newly heard entity

The RF icon turns blue and receives a blue badge when a previously unknown APRS station, object or item is inserted into the 15-entry history. Updates from an already-known identity do not increase the badge. Opening Heard Stations marks pending notifications as seen.

The counters are kept in RAM and reset at reboot.

## Tracker

The car uses the same APRS `/>` artwork as the station list.

- grey: periodic tracker disabled
- amber: tracker enabled but waiting for GPS/fix or another usable position
- green: tracker scheduling is active with a valid position

A one-shot BOOT beacon does not permanently light the tracker icon when periodic tracking is disabled.

## Stopar route logger

The save icon shows the state of the independent GPS route logger.

- grey: Stopar disabled
- amber: enabled but waiting for SD/GPS, automatically paused or manually paused
- green: route points are being recorded
- red: a directory or SD write error was latched

## Digipeater

The digipeater icon represents APRS `/#`.

- grey: RF digipeating disabled
- purple: RF digipeating enabled

## LoRa iGate

The gateway diamond with an `L` overlay is the same `L&` rendering used for LoRa iGate APRS entities.

- grey: receive-only iGate disabled
- amber: iGate enabled but Wi-Fi/APRS-IS login is not yet verified
- green: APRS-IS login verified and RF-to-IS gating is operational


## Hodiny od verze 2.3.0

- `--:--`: RTC ani GPS zatim neposkytly platne datum a cas.
- bile cislice: cas pochazi z PCF85063 RTC nebo z kratkodobeho holdoveru.
- zelene cislice: aktualni GPS datum a UTC cas jsou platne a pouzivaji se jako reference.
- RTC je ulozeno v UTC; zobrazeni pouziva CET/CEST a evropska pravidla letniho casu.
