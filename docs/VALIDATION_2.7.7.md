# Validation 2.7.7 - StationStore startup stack fix

## Fault reproduced

Firmware 2.7.6 panicked during `RadioService::begin()` with a loop-task stack canary violation. The decoded backtrace ended in `StationStore::clear()` at the aggregate assignment `view_ = ViewState{}`. The route-analysis additions increased `StationStore::ViewState` to 8,536 bytes in the host build, so the aggregate temporary exceeded the Arduino loop-task stack during startup.

## Correction

`StationStore::clear()` now destroys and placement-constructs `ViewState` directly in its existing static storage. No full-sized temporary is created on the task stack.

## Host verification

- APRS route-analysis test passes after the change.
- GCC stack-usage output reports 16 bytes for `StationStore::clear()` instead of a `ViewState`-sized temporary; `StationStore::ingest()` reports 672 bytes.
- Direct, repeated and subsequent direct receptions retain correct independent counters and last-direct time.

## Hardware verification

1. Upload firmware 2.7.7.
2. Confirm startup continues beyond `[GPS] UART2 listening...` and reaches LoRa/UI initialization.
3. Receive one direct packet and one packet with used `*` path elements.
4. Open station detail and verify route, hop count, last DIGI and both counters.
