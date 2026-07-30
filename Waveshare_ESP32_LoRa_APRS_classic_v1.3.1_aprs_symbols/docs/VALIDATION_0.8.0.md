# Validation notes for version 0.8.0

The following host-side checks were run while preparing this archive:

- `AprsCore` compiled with GCC using `-Wall -Wextra -Werror`.
- uncompressed tracker position encoding was parsed back to the original position.
- compressed Base-91 tracker position encoding was parsed back to the original position.
- invalid coordinates were rejected.
- distance and initial bearing were checked between a position near Plzen and Prague.
- the 15-entry APRS entity store and five-entry weather store were tested for uniqueness, newest-first ordering and oldest-entry replacement.
- fixed-interval tracker startup, configuration re-arm, compressed GPS packet generation, SmartBeacon corner timing and stale-fix waiting were tested with host-side service stubs.
- SettingsService, GpsService, AppController and all LVGL screen source files passed host-side syntax builds with interface stubs.
- the source tree was searched for CRC configuration: `radio_.setCRC(false)` is present and no `radio_.setCRC(true)` call is present.

A complete ESP32/Arduino link was not performed in the preparation environment because the PlatformIO ESP32 toolchain was not installed. Run a clean PlatformIO build before upload:

```powershell
Remove-Item -Recurse -Force .pio
pio run -e waveshare-esp32-release
pio run -e waveshare-esp32-release -t upload
pio device monitor -b 115200
```
