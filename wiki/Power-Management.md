# Power Management

The onboard AXP2101 provides battery and system telemetry.

## Available values

The Power page shows:

- battery presence
- battery percentage
- battery voltage
- charging, discharging or USB/standby state
- charger phase
- configured charging current
- target charging voltage
- USB/VBUS state and voltage
- system voltage
- internal PMIC temperature
- latest power-state transition

The displayed temperature is the **AXP2101 die temperature**, not the battery-cell temperature.

The displayed charging current is the configured charger limit, not a live measured device current.

## Persistent battery history

The graph stores up to 96 points.

A new point is accepted when:

- battery percentage changes by at least 1% and the value is confirmed twice
- power mode changes
- one hour passes without another accepted point

Graph colors:

- green: charging
- orange: discharging
- blue: USB/standby

The complete history is stored as a versioned CRC32-protected NVS blob and is restored after reset or power loss. Normal two-second PMIC polling does not write Flash.

## Display power policy

### USB-C operation

- 100% brightness
- no automatic dimming
- no automatic display blanking

### Battery operation

- configured brightness from 10% to 100%
- after 30 seconds: dim to 15%
- after selected timeout: backlight off

Available timeout options:

```text
Never, 30 s, 60 s, 2 min, 5 min
```

Only the LCD backlight is disabled. LoRa, GPS, Tracker, DIGI, iGate, Trail logger and other services continue running.

The first touch or BOOT press after full blanking wakes the display without performing its normal action.

## Charging current

The current firmware treats charger parameters as read-only telemetry. A higher charging current can be configured in code through AXP2101, but any change must respect:

- battery manufacturer limit
- connector and cable capability
- board temperature
- power-supply capability
- absence of direct battery-temperature sensing

A conservative 500 mA setting is generally more reasonable than 1000 mA for initial testing with a suitable 4000 mAh protected battery, but the battery datasheet remains authoritative.

## Measuring device current

The AXP2101 status available to this project indicates power direction and voltage values but does not provide a reliable numerical load-current value. Use an external current monitor such as INA219 or INA226 when live current, consumed mAh or power is required.
