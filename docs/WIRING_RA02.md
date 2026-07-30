# RA-02 wiring - classic Waveshare ESP32-Touch-LCD-3.5

| RA-02 | Waveshare header label | ESP32 GPIO |
|---|---|---:|
| 3.3V | 3V3 | - |
| GND | G | - |
| SCK | 14 | 14 |
| MISO | 13 | 13 |
| MOSI | 26 | 26 |
| NSS / CS | 33 | 33 |
| RESET | 32 | 32 |
| DIO0 | 2 | 2 |
| DIO1-DIO5 | not connected | - |

The RA-02 is on HSPI. The onboard LCD and microSD stay on VSPI, so LoRa and SD can operate at the same time.

Use 3.3 V only. Do not transmit without a 433 MHz antenna. Add local decoupling close to RA-02: 100 nF + 10 uF + 47-100 uF.

GPIO2 is a boot-strapping pin and is also shared with the optional onboard audio I2S interface. Add a 10 kOhm pulldown from GPIO2 to GND so DIO0 remains low during reset and firmware upload. Do not initialize audio while RA-02 DIO0 uses GPIO2. GPS RX uses GPIO4, so audio must remain disabled for that reason as well.
