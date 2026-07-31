# Complete Czech and English user interface

Firmware 2.6.0 uses one persistent language setting for the complete user interface.
The default is Czech and the alternative is English.

## Storage and startup

The selected `App::UiLanguage` value is stored in NVS under the existing key
`trklang`. Keeping the previous key makes upgrades from 2.5.0 backward compatible.
A missing or invalid value selects Czech.

`AppController` loads settings and calls `App::Localization::setLanguage()` before
display and service initialization. Splash and startup status messages therefore use
the saved language from the beginning of normal UI operation.

## Runtime switching

The **Jazyk rozhrani / Interface language** selector is on the Settings page. Saving a
new selection updates the global localization revision. `ScreenManager` detects the
revision and rebuilds the active screen immediately; a device restart is not required.

## Coverage

Localization covers menu labels and descriptions, all application screens, editor
hints, confirmations, validation errors and service-generated status/error messages:

- LoRa, GPS and radio diagnostics
- messages, stations, station detail and navigation
- weather list and weather detail
- Tracker and Trail logger
- map, astronomy and missing-tile/SD states
- power and display settings
- DIGI/iGate and APRS-IS connection states
- general Settings and startup/placeholder screens

The first comment line in each newly created Trail logger TXT file follows the selected
language.

## Intentionally untranslated data

The following content is protocol or measurement data and is intentionally left
unchanged:

- callsigns, SSIDs and raw TNC2/APRS frames
- raw NMEA sentences
- raw APRS-IS server responses
- standard abbreviations such as GPS, RSSI, SNR, SF, CR, BW and TX
- units and numeric values
- fixed LoRa compatibility values and packet contents

The current Montserrat build is ASCII-oriented. Czech UI strings therefore avoid
accented characters to prevent missing glyphs.

## Implementation

The central API is in:

```text
src/app/localization.h
src/app/localization.cpp
```

Typical use:

```cpp
App::Localization::text("Nastaveni", "Settings")
```

New user-visible text should always provide both Czech and English variants. Debug log
messages that are visible only on the serial console do not need localization unless
they are copied into a UI state field.
