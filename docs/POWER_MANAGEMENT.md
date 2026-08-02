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

From version 2.7.4 it displays a persistent battery-percentage trend with 96
points. A point is stored when a 1-percent change in either direction is
confirmed by two consecutive polls, when the operating mode changes, or after
one hour with no change. Green segments indicate active charging, orange segments discharging and
blue segments USB/standby operation. The x-axis is reconstructed from the stored
minute timestamps, so variable sampling intervals retain their real spacing.

The complete history is saved to a dedicated ESP32 NVS namespace as one
versioned blob protected by CRC32. It is restored after reset or power loss.
NVS is written only when a new history point is accepted; the normal two-second
PMIC polling path performs no Flash write.

The temperature belongs to the AXP2101 die. It is not a measurement of the
Li-Pol cell. The displayed current is the charger configuration value, not a
live current measurement.

## Main-loop priority

Power telemetry is processed after GPS input and the LoRa radio service. It is
polled before display-power policy so USB insertion can wake the backlight; the
tracker and Stopar continue in the same loop. AXP2101 values are read once every
two seconds. NVS access occurs only when a history point is added or restored at
startup; no filesystem access or blocking delay is used during ordinary polls.


## Rizeni podsviceni od verze 2.3.0

- USB-C: 100 % jasu, bez automatickeho vypnuti.
- Baterie: jas 10-100 % a timeout 0/30/60/120/300 sekund z NVS.
- Po 30 sekundach bez aktivity se jas snizi na 15 %; po nastavenem timeoutu se podsviceni vypne.
- Vychozi prubeh je 70 % po dobu 30 s, 15 % mezi 30-60 s a pote 0 %.
- Pri timeoutu `Nikdy` zustane displej po 30 s ztlumeny na 15 %, ale nevypne se.
- Vypina se pouze podsviceni LCD; vsechny radio/GPS/sluzby zustavaji aktivni.
- Prvni dotyk po zhasnuti se pouzije pouze k probuzeni a neni predan LVGL.
- BOOT stisk pri zhasnuti se spotrebuje jako probuzeni bez rucniho beaconu.
- Pri chybe AXP2101 je bezpecny fallback plny jas bez automatickeho vypnuti.
