# APRS directed messages

Version 0.9.0 adds a fixed-memory directed-message subsystem.

## On-air TNC2 format

The TNC2 separator after the destination is followed by the APRS `:` data type
identifier, therefore two adjacent colons are visible before the padded
addressee:

```text
OK5TVR-15>APRS::OK1ABC-7 :Ahoj{003
```

The addressee field is exactly nine characters. The message text is at most 67
printable ASCII characters and cannot contain `|`, `~` or `{`. The optional
identifier after `{` is one to five alphanumeric characters.

Acknowledgement and rejection frames are:

```text
OK1ABC-7>APRS::OK5TVR-15:ack003
OK1ABC-7>APRS::OK5TVR-15:rej003
```

## Runtime behavior

- 20 newest incoming/outgoing entries are held in RAM.
- Outgoing messages always receive a three-digit identifier.
- Messages addressed to the configured callsign are acknowledged automatically.
- `ALL`, `QST` and `CQ` are displayed but never acknowledged.
- Duplicate source/identifier pairs update one row and schedule another ACK.
- Pending messages are transmitted immediately and retried after 10, 30, 60
  and 120 seconds. After the fifth transmission a final 30-second ACK grace
  period is used before the entry changes to `Failed`.
- ACK traffic has priority over new/retried user messages. Tracker packets wait
  while the radio is transmitting.
- Reception and scheduling run in `RadioService::update()` on every main-loop
  pass, independently of the visible screen.
