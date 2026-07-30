# Validation notes for 0.9.0

Host-side C++17 checks were run with `-Wall -Wextra -Werror` for:

- directed-message TNC2 build and parse
- exact nine-character addressee padding
- ACK build and parse
- third-party wrapped message parsing
- rejection of reserved message characters
- duplicate incoming message merging
- automatic ACK queueing
- outgoing retry timing
- incoming ACK state transition
- group-message storage without ACK

The source tree was searched to confirm that LoRa payload CRC remains disabled
with `radio_.setCRC(false)` and no `radio_.setCRC(true)` call is present.

A complete ESP32 PlatformIO build could not be run in the generation environment
because the ESP32 PlatformIO toolchain was not available. A clean local build is
therefore required before upload.
