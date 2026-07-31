# Offline mapa ve verzi 2.1.0

Firmware zobrazuje mapu bez internetu z dlazdic ulozenych na microSD karte.
Mapova obrazovka je aktivni pouze pri otevrene polozce **Offline mapa**; mimo ni
se SD dlazdice nectou.

## Funkce

- standardni Web Mercator / XYZ souradnice dlazdic
- zoom 3 az 18, vychozi zoom 13
- stred podle aktualni GPS polohy; bez GPS se pouzije vychozi poloha z Nastaveni
- automaticke posunuti mapy po pohybu stredu alespon o 64 obrazovych bodu v rezimu GPS/DEF
- rucni posouvani mapy tazenim prstu; po tahu se aktivuje rezim `MAN`
- plynuly nahled pri tahu bez opakovaneho cteni SD; nove dlazdice se nactou az po uvolneni
- zobrazeni vlastni polohy modrym bodem
- zobrazeni vsech APRS stanic, objektu a polozek s jejich skutecnym APRS symbolem
- cervene oramovani entity s emergency priznakem
- oranžova stopa poslednich 64 bodu aktualniho/posledniho zaznamu Stopare
- pocitadlo chybejicich dlazdic a postup nacitani

## Ovladani

- **Tazeni prstem**: rucni posun mapy v libovolnem smeru
- **Nahoru**: priblizit mapu
- **Dolu**: oddalit mapu
- **OK**: opustit rucni rezim, znovu zapnout sledovani GPS/vychozi polohy a vycentrovat mapu
- **Zpet**: hlavni menu

Posun se povazuje za tah az od sesti pixelu. Behem tahu se pouze vizualne
posune hotovy framebuffer a vsechny prekryvy. Po uvolneni prstu se rozdil
prevede na novy geograficky stred, rozpracovane cteni stareho vyrezu se zrusi a
nove dlazdice se nactou progresivne. Tim nevznika zapis na SD ani opakovane
otvirani souboru pri kazdem dotykovem vzorku.

Stavovy radek pouziva:

- `GPS`: mapa automaticky sleduje platny GPS fix
- `DEF`: mapa automaticky sleduje vychozi polohu z Nastaveni
- `MAN`: stred byl zvolen rucnim posunem; nove GPS polohy mapu neposunou

## Format na SD karte

Firmware nepouziva PNG/JPEG dekoder. Dlazdice jsou ulozeny jako jednoduche
RGB565 soubory, aby se cetly po malych blocich a nezatizovaly LoRa prijem.

```text
/MAP/<zoom>/<x>/<y>.rgb
```

Priklad:

```text
/MAP/13/4398/2785.rgb
```

Kazdy soubor musi mit presne:

```text
256 x 256 x 2 = 131072 bajtu
```

Barevny format je little-endian RGB565 a odpovida konfiguraci:

```cpp
LV_COLOR_DEPTH = 16
LV_COLOR_16_SWAP = 0
```

## Prevod existujicich XYZ dlazdic

Projekt obsahuje nastroj:

```text
tools/convert_map_tiles.py
```

Vyžaduje Python a Pillow:

```powershell
python -m pip install Pillow
python tools/convert_map_tiles.py `
  --input C:\mapy\xyz `
  --output E:\MAP `
  --min-zoom 10 `
  --max-zoom 16
```

Vstup musi mit strukturu:

```text
C:\mapy\xyz\13\4398\2785.png
```

Vystup lze zkopirovat primo do korenove slozky `MAP` na SD karte.
Existujici soubory se standardne neprepisuji; pouzijte `--overwrite`, pokud je
chcete nahradit.

Pouzivejte pouze mapova data, ktera smite stahovat a ukladat offline. Firmware
sam zadne dlazdice z internetu nestahuje.

## Ochrana LoRa provozu

LCD a SD sdileji VSPI, ale RA-02 pouziva samostatnou HSPI sbernici. Nacitani
mapy probiha az po obsluze GPS, LoRa, trackeru a Stopare. V jednom pruchodu
hlavni smyckou se z SD nacte nejvyse osm radku jedne dlazdice. Jedna viditelna
mapa obvykle pouziva ctyri az sest dlazdic.

Pri opusteni mapove obrazovky se otevreny soubor dlazdice zavre a dalsi nacitani
se zastavi. Pri navratu se aktualni vyrez nacte znovu. Chybejici nebo poskozeny
soubor se zobrazi jako seda sachovnice a nezastavi ostatni funkce terminalu.

## Pamet

Mapovy framebuffer ma rozmery 480 x 202 bodu a zabira 193920 bajtu. Alokuje se pouze v PSRAM, aby mapa neodebrala velkou cast interni RAM. Pokud
PSRAM nema dostatek volneho mista, mapa se oznaci jako nedostupna, ale LoRa, GPS, tracker a ostatni obrazovky
zustanou funkcni.
