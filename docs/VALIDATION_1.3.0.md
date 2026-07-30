# Validation 1.3.0

## Host and static checks

- Compile `TxQueue` with strict warnings and verify priority/FIFO selection.
- Verify a second scheduled tracker packet replaces the older queued tracker.
- Fill the queue with test packets and verify an ACK evicts one low-priority item.
- Re-run geometry, notification-store, DIGI core, GPS capture, power and Stopar tests.
- Compile changed radio, tracker, screen-manager and station UI sources against API stubs with `-Wall -Wextra -Werror`.
- Confirm no pin, baud-rate or I2S configuration changed from version 1.2.5.

## Device TX scheduler test

1. Open **LoRa APRS / diagnostika** and verify queue depth is zero in idle RX.
2. Trigger the test packet repeatedly while the radio transmits. Verify requests enter the queue and RX resumes after every TX.
3. Enable tracker and send an APRS message. Verify the message/ACK transmits before a waiting scheduled tracker packet.
4. Enable DIGI and create simultaneous tracker, DIGI and test traffic. Verify the order shown by `last TX source` follows the documented priorities.
5. Keep the radio busy until multiple tracker intervals occur. Verify only the newest scheduled tracker frame remains queued and the `nahrazeno` counter increases.
6. Verify queue depth never exceeds 8 and dropped/evicted items increment `drop` without a crash or memory growth.

## Automatic recovery test

1. Boot once with RA-02 disconnected. Verify recovery attempts appear at intervals no shorter than five seconds and the UI remains responsive.
2. Reconnect/reset the RA-02 and verify the next attempt restores RX without rebooting the ESP32.
3. Create or simulate a TX timeout and verify `TX timeout` plus one successful/failed recovery entry.
4. Confirm GPS, touch, Stopař and power telemetry continue during radio recovery.
5. Confirm lifetime RX/TX/error counters do not return to zero after a successful recovery.

## Station detail and navigation test

1. Receive at least three APRS position packets from unique entities.
2. Open **Prijate stanice**, use Up/Down and verify the highlighted row follows selection and scrolls into view.
3. Press OK and verify entity type/name, owner callsign, age, packet count, position, RSSI, SNR and last TNC2 frame.
4. Press OK again and verify distance and true bearing update from the live GPS reference.
5. Remove the GPS fix and verify navigation switches to `vychozi poloha` rather than crashing.
6. Select an entity without position and verify navigation is disabled with a clear message.
7. Receive a new position for the currently open entity and verify detail/navigation refresh to the updated target.

## Required embedded build

```powershell
pio run -e waveshare-esp32-release
```

Then upload and perform the device tests above. The validation container does not contain PlatformIO, so the final ESP32 linker/build step must be performed on the development computer.
