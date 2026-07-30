# Validation 1.2.3

- Full CRLF NMEA sentence capture.
- LF-only and CR-only sentence termination.
- Embedded NUL/control bytes are ignored.
- A lone `$` does not overwrite the last completed sentence.
- GPS diagnostic label has an explicit 452 x 42 px wrapping area.
