# Development and Architecture

## Source layout

```text
src/drivers   direct hardware access
src/services  radio, APRS and application logic
src/ui        LVGL screens and navigation
src/app       startup, orchestration and shared types
lib/AprsCore  hardware-independent APRS functions
```

## Main-loop order

The single-threaded application is intentionally scheduled so radio handling remains early:

1. LVGL and hardware buttons
2. GPS input and RTC synchronization
3. LoRa RX/TX, APRS parsing, DIGI and iGate networking
4. Web OTA/AP+STA maintenance
5. power, tracker, Trail logger, astronomy and map services
6. system diagnostics sampling
7. visible-screen refresh

## SPI allocation

- VSPI: LCD and microSD
- HSPI: external RA-02

This separation prevents map and log storage from sharing the LoRa SPI controller.

## Memory model

- LVGL objects prefer PSRAM
- display draw buffer uses internal DMA-capable RAM
- map framebuffer uses PSRAM
- TX queues and message/station stores use fixed-capacity structures
- dynamic allocation is avoided in radio-critical paths

### StationStore stack-safety rule

`StationStore::ViewState` is large because it includes complete APRS paths and TNC2 frames for 15 entities. Do not initialize it using a large temporary assignment such as:

```cpp
view_ = ViewState{};
```

On the Arduino loop task, that may create a temporary object on the stack. The current implementation reconstructs the object directly in its existing global storage.

## Central TX queue

Priority order:

1. APRS message ACK
2. outgoing APRS message
3. DIGI packet
4. manual BOOT beacon
5. scheduled tracker
6. test packet

The queue is fixed at eight entries. Scheduled tracker frames may be coalesced, and high-priority traffic may evict a lower-priority item.

## Radio recovery

The SX1278 is reinitialized after:

- startup initialization failure
- driver error state
- three consecutive RX read failures
- 15-second TX timeout

Recovery is rate-limited and resets only the LoRa radio path.

## Localization

All user-visible application text should use:

```cpp
App::Localization::text("Czech text", "English text")
```

Protocol data and serial-only debug strings do not require translation.

## Adding documentation

User-facing English Wiki source lives in `/wiki`. Changes pushed to the configured branch are synchronized to the GitHub Wiki by `.github/workflows/publish-wiki.yml`.

See `docs/GITHUB_WIKI.md` for one-time repository setup and local publishing options.

## Stabilization services in v2.7.8

`SystemDiagnosticsService` samples internal heap, largest blocks, historical minimum heap, PSRAM, loop-task stack reserve, uptime and the ESP32 reset reason. The UI receives a small snapshot once per second.

`OtaService` writes only to the inactive A/B application slot. It validates the ESP32 image header and application descriptor before starting Flash writes. Invalid or interrupted uploads are aborted without scheduling a restart. The iGate Wi-Fi path preserves AP+STA mode so APRS-IS reconnects do not remove the OTA access point.

APRS path parsing classifies direct RF, used RF digipeaters and Internet transport separately. The fixed-size station store therefore supports D/hop/I/? badges without dynamic allocation.
