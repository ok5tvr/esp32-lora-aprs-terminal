# LoRa Radio and Diagnostics

## LoRa APRS page

The LoRa APRS page reports the active radio profile and runtime state, including:

- radio online/offline state
- RX/TX mode
- active frequency, bandwidth, spreading factor and coding rate
- output power
- latest RSSI, SNR and frequency error
- raw packet and APRS decode counters
- TX queue depth, high-water mark, drops and replacements
- current TX source
- radio recovery attempts and results

## Czech APRS profile

The default compatible profile is:

```text
433.775 MHz
Bandwidth 125 kHz
SF12
CR 4/5
TX power 10 dBm
Sync word 0x12
Explicit header
Preamble 8
RadioLib payload CRC disabled
```

The disabled RadioLib payload CRC is intentional for compatibility with the deployed LoRa APRS profile. APRS/TNC2 structure is still validated by the parser.

## Safe profile changes

A custom profile is stored in NVS. When settings change, the firmware waits until:

- no transmission is active
- the central TX queue is empty

It then reinitializes only the RA-02. GPS, display, map, Trail logger and network services remain active.

## Background channel RSSI

The **Diagnostics** page stores 20 measurements at five-minute intervals.

Each point contains:

- average of eight RSSI readings
- strongest RSSI reading in the sample window

A measurement starts only when the SX1278 is in receive mode and the TX queue is empty. It is postponed if transmission begins.

Less negative RSSI means stronger channel activity. For example, `-80 dBm` is stronger than `-120 dBm`.

The graph is not a calibrated spectrum analyser or channel-occupancy measurement. It may contain receiver noise, other signals or valid LoRa packets.

The radio diagnostics history remains in RAM and resets after reboot. It is cleared when the active RF profile changes so different frequencies are not mixed.

## Automatic recovery

The firmware reinitializes only the SX1278 path after:

- startup initialization failure
- driver error state
- three consecutive RX read failures
- TX timeout

Attempts are rate-limited. Packet counters and queued transmissions are retained where possible.
