# Validation notes for 0.9.2

Host-side C++17 checks were run with `-Wall -Wextra -Werror` for:

- six-character Maidenhead conversion of the configured default position to `JN69PS`
- conversion of the equator/Greenwich position to `JJ00AA`
- rejection of invalid coordinates
- preservation of the existing distance and bearing calculation

The GPS service now independently tracks serial bytes, complete NMEA sentences and
checksum-valid packets. This permits the UI to distinguish wiring/baud problems from
invalid NMEA data and from a valid receiver waiting for a fix.

The source tree was searched to confirm that LoRa payload CRC remains disabled with
`radio_.setCRC(false)` and no `radio_.setCRC(true)` call is present.

A complete ESP32 PlatformIO build could not be run in the preparation environment
because the ESP32 PlatformIO toolchain was not installed. Run a clean local build before
upload.
