# Weather and Astronomy

## APRS weather stations

The firmware keeps a list of the five most recently heard unique APRS weather stations.

Depending on the received packet, the detail page can show:

- temperature
- humidity
- pressure
- wind direction and speed
- gust
- rain values
- solar radiation
- station position
- RSSI and SNR
- latest TNC2 frame

Missing fields are left unavailable rather than inferred.

## Astronomy page

The Astronomy page works completely offline and displays:

- sunrise
- sunset
- daylight duration
- current Sun altitude
- moonrise
- moonset
- Moon phase
- illuminated fraction
- approximate lunar age

Inputs:

- local civil date from the RTC/GPS time service
- current GPS position, or saved default position

The result is recalculated only when needed, such as a date change, reference-source change or significant movement.

Astronomical values are appropriate for a field information display but are approximate. Terrain, buildings, atmospheric refraction and the local horizon may shift observed rise and set times by several minutes.
