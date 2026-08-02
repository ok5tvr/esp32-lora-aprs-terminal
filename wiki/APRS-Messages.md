# APRS Messages

The terminal supports directed APRS text messages with automatic acknowledgements and retry handling.

## Message format

Example outgoing TNC2 frame:

```text
OK5TVR-15>APRS::OK1ABC-7 :Hello{003
```

The addressee field is exactly nine characters. The message text is limited to printable ASCII and a maximum of 67 characters. The characters `|`, `~` and `{` are not accepted in normal message text.

ACK and rejection examples:

```text
OK1ABC-7>APRS::OK5TVR-15:ack003
OK1ABC-7>APRS::OK5TVR-15:rej003
```

## Runtime behavior

- 20 newest incoming and outgoing entries are kept in RAM
- outgoing messages receive a three-digit ID
- messages addressed to the local callsign are acknowledged automatically
- `ALL`, `QST` and `CQ` are displayed but not acknowledged
- duplicate source/ID messages update one row and schedule another ACK
- ACK packets have the highest TX priority

## Retry schedule

A pending message is sent immediately and retried after approximately:

```text
10 s, 30 s, 60 s and 120 s
```

After the fifth transmission, a final ACK grace period is used before the state becomes **Failed**.

## Using the Messages screen

The page contains the recent conversation history and a touchscreen message composer. Enter:

- destination callsign with optional SSID
- message text

Send only short operational information. APRS is a shared low-rate radio channel and should not be treated as a private chat service.

## Persistence

Message history is held in RAM and resets after reboot. Callsign and service settings remain persistent in NVS.
