# Hardware and Wiring

## Supported board

The firmware targets the classic Waveshare ESP32-Touch-LCD-3.5 board with:

- ESP32-D0WDR2-V3
- 3.5-inch ST7796 display
- FT6336 capacitive touch controller
- 16 MB Flash
- 2 MB PSRAM
- onboard microSD slot
- AXP2101 power-management IC
- PCF85063 RTC

It is not a drop-in build for the ESP32-S3 edition.

## Onboard VSPI bus

The LCD and microSD share VSPI data lines and use separate chip selects.

| Signal | GPIO |
|---|---:|
| SCK | 18 |
| MISO | 19 |
| MOSI | 23 |
| LCD CS | 5 |
| SD CS | 15 |
| LCD DC | 27 |

Do not move the external LoRa module onto GPIO18, GPIO19 or GPIO23.

## RA-02 / SX1278 wiring

The RA-02 uses independent HSPI.

| RA-02 pin | ESP32 GPIO |
|---|---:|
| SCK | 14 |
| MISO | 13 |
| MOSI | 26 |
| NSS / CS | 33 |
| RESET | 32 |
| DIO0 | 2 |
| DIO1 | not connected |
| VCC | 3.3 V only |
| GND | GND |

### Required precautions

- Connect a 433 MHz antenna before any transmission.
- Add `100 nF + 10 uF + 47-100 uF` between 3.3 V and GND close to the RA-02.
- Add a 10 kOhm pulldown from GPIO2 to GND so DIO0 stays low during reset and upload.
- Keep the optional onboard I2S audio interface disabled because GPIO2 and GPIO4 are used by LoRa and GPS.

## GPS wiring

The firmware uses UART2 in receive-only mode.

| GPS signal | ESP32 connection |
|---|---:|
| TX | GPIO4 |
| RX | not connected |
| GND | GND |
| VCC | according to GPS breakout specification |

The signal entering GPIO4 must use **3.3 V logic**. Do not connect a raw 5 V UART output directly to the ESP32.

Default serial settings:

```text
9600 baud, 8 data bits, no parity, 1 stop bit
```

## I2C devices

The onboard I2C bus uses:

| Signal | GPIO |
|---|---:|
| SDA | 21 |
| SCL | 22 |

The bus is shared by the touch controller, AXP2101 PMIC, RTC and other onboard devices.

## Battery and USB power

The board may be powered from USB-C or a protected single-cell Li-Pol/Li-ion battery.

The firmware reads battery voltage, percentage, charger state, VBUS voltage, system voltage and PMIC temperature. The AXP2101 does **not** provide a reliable live device-current measurement in this implementation.

A 4000 mAh battery may support a higher charge current than 200 mA, but the permitted current must be confirmed from the battery specification. The battery capacity alone does not define the safe charging current.

## microSD card

Use a reputable FAT32-formatted card. It is used for:

- offline map tiles under `/MAP`
- Trail logger files under `/STOPAR`

Insert the card before startup. Avoid removal during active logging or map access.
