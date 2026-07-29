# Onboard buttons

Firmware 1.0.3 uses the onboard **BOOT** button as a one-shot APRS position
beacon trigger.

- BOOT is read on classic ESP32 `GPIO0` with the internal pull-up enabled.
- The input is active low and is polled with software debounce.
- A short press of roughly 40-1500 ms creates one manual beacon request.
- A held button at firmware start is ignored until it is released.
- Repeated presses within one second are ignored.
- RESET remains a hardware reset and cannot be handled by the application.
- PWR retains its power-management function and is not reassigned.

The manual beacon uses the saved tracker source, packet format and symbol. It
works even when periodic tracking is disabled. With GPS selected, the request
waits up to 15 seconds for a valid fix. With Default selected, the saved default
coordinates are used immediately. If the radio is busy, the request waits until
it becomes free or until the timeout expires.

Do not hold BOOT while powering or resetting the board, because `GPIO0` is also
the ESP32 download-mode strapping input.
