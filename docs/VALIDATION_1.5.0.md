# Validace 1.5.0

## Provedene staticke a hostitelske kontroly

- overena konfigurace verze `1.5.0`, intervalu 300 s a kapacity 20 bodu
- `TxQueue`, geometrie, DIGI/iGate jadro, notifikace, GPS, power management a Stopar prosly hostitelskymi testy
- nova obrazovka Diagnostika prosla syntaktickou kontrolou s API LVGL 8.4
- overeno pouziti `SX1278::getRSSI(false, true)` pro aktualni RSSI bez prepnuti LoRa prijmu
- `LV_USE_CHART` je v `lv_conf.h` zapnuto explicitne
- plny embedded build je nutne dokoncit v PlatformIO na vyvojovem pocitaci

## Sestaveni

```powershell
pio run -e waveshare-esp32-release
```

## Nastaveni

1. Otevrit **Nastaveni**.
2. Overit text `Firmware: LoRa APRS Terminal v1.5.0`.
3. Overit, ze editace CALL a souradnic i ukladani do NVS zustaly funkcni.

## Diagnostika RSSI

1. Otevrit **Diagnostika** do 15 sekund po startu.
2. Overit odpocet do prvniho mereni.
3. Po prvnim bodu overit prumer, spicku a obe krivky grafu.
4. Overit novy bod po peti minutach.
5. Pro zrychleny test docasne zmenit `RADIO_NOISE_SAMPLE_INTERVAL_MS` na 5000.
6. Overit, ze po 20 bodech zustava pocet 20 a nejstarsi bod se odstrani.

## Soubezny RF provoz

1. Behem mereni prijmout APRS paket a overit, ze RX zustava funkcni.
2. Zaradit testovaci TX nebo beacon; rozdelany burst se musi zrusit a opakovat.
3. Overit LoRa RX, TX, tracker, DIGI, zpravy a automatickou obnovu radia.
4. Overit, ze historie je pouze v RAM a restart ji vynuluje.
