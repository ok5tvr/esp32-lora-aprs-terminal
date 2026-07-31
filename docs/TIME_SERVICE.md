# RTC and GPS time service

Firmware 2.3.0 adds `TimeService` for the onboard PCF85063 RTC.

## Source priority

1. valid GPS UTC date and time
2. PCF85063 RTC
3. software holdover from the most recent valid reference

The RTC is read during startup. GPS time does not require a valid position fix;
a valid NMEA date and time is sufficient. When GPS becomes valid, its UTC value
updates the running clock and is written to the RTC. Further corrections are
limited to one per six hours. If the first write fails, it is retried after one
minute instead of on every main-loop pass.

## Storage and display

The PCF85063 stores UTC. The main-menu header displays local Czech time:

- CET, UTC+1 in winter
- CEST, UTC+2 in summer
- summer time starts on the last Sunday in March at 01:00 UTC
- summer time ends on the last Sunday in October at 01:00 UTC

The main-menu clock replaces the former `LoRa` title. Green digits indicate a
currently valid GPS reference. White digits indicate RTC or holdover. If no
valid source exists, `--:--` is displayed.

## Hardware

- I2C SDA: GPIO21
- I2C SCL: GPIO22
- PCF85063 7-bit address: `0x51`

The time service uses short I2C transfers and does not poll the RTC in every
loop. During GPS loss the RTC is refreshed at most once per minute.

## Chovani pri neplatnem RTC

Pokud RTC odpovi, ale obsahuje neplatne datum/cas nebo priznak zastaveneho
oscilatoru, firmware ceka na GPS. RTC se pri tom znovu kontroluje nejvyse jednou
za 60 sekund, aby neblokovalo sdilenou I2C sbernici v kazdem pruchodu smyckou.
