# Validace 2.0.0 - offline mapa

## Automaticke a hostitelske kontroly

- Web Mercator projekce: rovnik, stred obrazovky, pohyb na vychod a prechod pres datovou hranici
- syntaxe `MapService`, `MapScreen`, `ScreenManager` a `AppController` s prisnymi volbami `-Wall -Wextra -Werror`
- postupne nacteni dlazdic a dokonceni stavoveho automatu mapy
- zoom 13 -> 14 a centrovani projekce
- prevod testovaci PNG dlazdice na presne 131072bajtovy RGB565 soubor
- zachovani poslednich bodu Stopare po ukonceni zaznamu
- regresni testy geometrie, TX fronty, APRS/DIGI jadra, notifikaci, GPS, power managementu a Stopare

## PlatformIO build

```powershell
pio run -e waveshare-esp32-release
```

## Priprava SD

1. Vytvorit nebo prevest alespon ctyri XYZ dlazdice v okoli vychozi polohy.
2. Zkopirovat je do `/MAP/<z>/<x>/<y>.rgb`.
3. Overit, ze kazdy soubor ma 131072 bajtu.
4. Vlozit kartu pred zapnutim terminalu.

## Funkcni test

1. Otevrit **Offline mapa**.
2. Overit zobrazeni dlazdic, modre vlastni polohy a text `Z13`.
3. Tlacitkem Nahoru zmenit zoom na 14; tlacitkem Dolu se vratit.
4. Tlacitkem OK vynutit nove centrovani.
5. Prijmout polohovy APRS paket a overit zobrazeni spravne APRS ikony.
6. Spustit Stopare a overit oranžovou stopu po alespon dvou bodech.
7. Odstranit jednu dlazdici a overit sedou sachovnici a pocitadlo `chybi`.
8. Behem nacitani prijmout a odeslat LoRa paket; RX/TX musi zustat funkcni.
9. Odejit z mapy a overit, ze tracker, DIGI/iGate a Stopar pokracuji.

## Omezeni

- mapa je pouze offline a nestahuje data pres Wi-Fi
- podporuje pouze XYZ Web Mercator
- prvni verze nema dotykove rucni posouvani
- stopa na mape obsahuje nejvyse poslednich 64 bodu v RAM
- plny embedded build a test na fyzicke desce je nutne dokoncit v PlatformIO
