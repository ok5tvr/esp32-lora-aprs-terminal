# Troubleshooting

## Device repeatedly resets

Open the serial monitor at 115200 baud and capture the complete panic and backtrace.

### Stack canary in `StationStore::clear()`

Firmware v2.7.6 could report:

```text
Stack canary watchpoint triggered (loopTask)
StationStore::clear()
```

Upgrade to v2.7.7 or later. The fix reconstructs the large station state directly in global storage instead of creating an approximately 8.5 kB temporary object on the Arduino loop-task stack.

If the installed firmware crashes before OTA starts, upload the corrected build through USB.

## RA-02 does not initialize

Check:

- 3.3 V supply, never 5 V
- all HSPI wires
- NSS on GPIO33
- RESET on GPIO32
- DIO0 on GPIO2
- 10 kOhm pulldown from GPIO2 to GND
- local decoupling capacitors
- antenna connected
- onboard audio disabled

The firmware attempts automatic RA-02-only recovery after initialization failure, repeated RX errors or TX timeout.

## No APRS packets are decoded

Verify:

- correct frequency and LoRa profile
- antenna and RF path
- radio status is RX
- packet counter versus APRS decode counter
- expected LoRa APRS framing
- local channel activity on **Diagnostics**

A rising raw RX count with no APRS decode may indicate incompatible packet contents rather than an RF problem.

## GPS shows traffic but no fix

Check the **GPS receiver** page:

- bytes received
- complete NMEA sentence
- valid checksum
- satellites and HDOP
- current date/time

Ensure GPS TX is connected to GPIO4 at 3.3 V logic and 9600 baud.

## microSD not detected

- format the card as FAT32
- insert it before startup
- use a smaller reputable card for testing
- inspect serial initialization messages
- check that LCD and SD remain on the onboard VSPI pins

## Map displays grey checkerboards

The required tile is missing, has the wrong path, has the wrong dimensions or is not raw little-endian RGB565.

Expected path:

```text
/MAP/<z>/<x>/<y>.rgb
```

Expected size:

```text
131072 bytes
```

## Trail logger enters Error

Correct the SD-card or directory problem, then disable and re-enable the Trail logger. A latched error intentionally prevents repeated file creation attempts.

## APRS-IS login is unverified

- check internet access and Wi-Fi credentials
- verify server and port
- use a valid APRS-IS passcode for the configured callsign
- `-1` is an unverified login and cannot activate the iGate
- avoid explicit callsign SSID `-0`

## OTA page is unavailable

- enable OTA in Settings and press Save
- connect to `LoRa-APRS-OTA`
- open `http://192.168.4.1`, not HTTPS
- verify that the running build contains the OTA partition layout
- use USB when the firmware crashes before OTA initialization

## Display appears off but radio continues

This is normal battery power-saving behavior. The firmware turns off only the backlight. Touch once to wake it; the wake touch is not passed to the active button.

## Battery current is not shown

The firmware can show charging state and configured charging limit, but not reliable live load current. Add an external INA219/INA226-class current monitor for numerical current measurement.
