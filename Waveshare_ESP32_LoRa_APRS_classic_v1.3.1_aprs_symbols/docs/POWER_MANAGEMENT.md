# AXP2101 power management

Firmware version 1.2.0 adds read-only power telemetry for the onboard AXP2101.
The PMIC shares the board I2C bus on GPIO21 (SDA) and GPIO22 (SCL). GPIO35 is
reserved for the AXP2101 interrupt output, but this release detects state
changes by light polling every two seconds and does not require the IRQ line.

## Safety boundary

`PowerService` does not alter:

- configured charging current
- charging target voltage
- DCDC/LDO output settings
- power-off behaviour

The configured charging current and target voltage are read back only for
presentation on the screen.

## Header

The right side of every normal screen header contains a compact summary:

```text
74%  3,91V  [symbol]
```

Symbol and colour priority:

1. critical accumulator: red battery symbol
2. active charging: green lightning symbol
3. USB-C connected without active charging: blue USB symbol
4. accumulator operation: white battery-level symbol

The default critical limits are 10 percent or 3.40 V. They are defined in
`include/app_config.h`.

## Power page

The main-menu item **Napajeni** displays battery presence, percentage, battery
voltage, operating state, charger phase, configured charge current, charge
target voltage, USB/VBUS state, VBUS voltage, system voltage, internal PMIC
temperature and the latest detected state transition.

The temperature belongs to the AXP2101 die. It is not a measurement of the
Li-Pol cell. The displayed current is the charger configuration value, not a
live current measurement.

## Main-loop priority

Power telemetry is processed after GPS input, LoRa radio service, APRS tracker
and Stopar. AXP2101 values are read once every two seconds. No filesystem access
or blocking delay is used by `PowerService`.
