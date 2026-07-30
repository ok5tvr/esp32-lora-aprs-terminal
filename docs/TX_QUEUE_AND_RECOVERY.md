# Central TX queue and automatic radio recovery

## Queue model

The queue is a fixed eight-entry array in internal RAM. Each entry contains the
complete on-air payload, source, priority, timestamp, sequence and optional APRS
message token. No `String`, `new`, STL container or FreeRTOS task is used.

| Priority | Source | Behaviour |
|---:|---|---|
| 0 | message ACK | highest priority |
| 1 | outgoing APRS message | retry token starts only at actual TX start |
| 2 | DIGI | random DIGI delay is applied before entering this queue |
| 3 | manual BOOT beacon | never coalesced |
| 4 | scheduled tracker | newest frame replaces older queued tracker |
| 5 | test packet | lowest priority |

A minimum 180 ms gap is kept between TX starts. RX processing and packet parsing
run before queue service on every main-loop pass.

## Recovery triggers

The SX1278 is reinitialized when:

- initial radio setup is offline,
- the driver reports an error mode,
- three consecutive `readData()` operations fail,
- a transmission exceeds the 15-second timeout.

Recovery attempts are separated by at least five seconds. Only the RA-02/HSPI
radio path is reset; GPS, UI, SD, NVS, Wi-Fi and PMIC services continue. Driver
packet/error counters are restored after reinitialization and the pending TX
queue remains intact.
