# Radio and system diagnostics

Firmware v2.7.8 combines the LoRa background-RSSI graph with live ESP32 resource diagnostics.

## Background RSSI history

- measurements use the currently active LoRa frequency
- the first automatic sample is taken about 15 seconds after startup
- later points are stored every 300 seconds
- each point contains the average and peak of eight RSSI reads separated by 25 ms
- the RAM history contains at most 20 points
- the history is cleared after a successful RF profile change
- the RSSI history is not restored after reboot

A measurement starts only while the SX1278 is in receive mode and the central TX queue is empty. If transmission starts, the sample is cancelled and retried later.

Less negative RSSI means stronger channel activity. The graph is not a calibrated spectrum analyser and may include receiver noise, unrelated signals or valid LoRa packets.

## System values

The Diagnostics screen refreshes the following values once per second:

- free internal heap
- largest contiguous internal-heap block
- historical minimum free internal heap
- free and largest PSRAM block
- minimum observed free stack reserve of the Arduino `loopTask`
- stored station count
- TX queue depth and high-water mark
- device uptime
- last ESP32 reset reason
- current OTA status and connected OTA client count

The stack value is derived from the FreeRTOS high-water mark and is displayed in bytes. It is a minimum observed reserve, not the current stack use.

## Warning thresholds

The interface highlights:

- internal heap below 32 KB
- loop-task stack reserve below 2 KB
- missing PSRAM

A decreasing historical minimum heap or stack reserve during repeated navigation may indicate a leak, excessive temporary object or unexpectedly deep call path.

## Restart reasons

Reported reasons include power-on, software reset, panic/watchdog, deep sleep, brownout and SDIO reset where supported by the installed ESP-IDF version. The numeric ESP reset code is retained alongside the human-readable text.
