# Validace 2.4.0 - nastaveni LoRa modulu

## Automaticke a hostitelske testy

- vychozi stav bez ulozenych hodnot je profil CZE APRS: 433.775 MHz, BW 125 kHz, SF12, CR 4/5 a 10 dBm
- profil CZE pri ukladani ignoruje pripadne predane vlastni hodnoty a vzdy normalizuje parametry na CZE
- vlastni profil prijme frekvenci 410-525 MHz, podporovanou sirku pasma, SF7-12, CR 4/5-4/8 a vykon 2-17 dBm
- neplatna vlastni frekvence 600 MHz je odmitnuta bez zmeny ulozene konfigurace
- vlastni profil 433.900 MHz / BW250 / SF9 / CR4/6 / 14 dBm se ulozi do NVS a po novem nacteni zustane zachovan
- navrat na profil CZE obnovi vsechny vychozi RF parametry
- zmenene zdrojove soubory prochazeji `clang++ -fsyntax-only -Wall -Wextra -Werror` proti hostitelskym stubum
- `include/lora_profile.h` prochazi samostatnym testem validniho CZE profilu a odmitnuti frekvence mimo rozsah

## Test na zarizeni

1. Po cistem startu otevrit **Nastaveni** a overit profil `CZE APRS`.
2. Na strance **LoRa APRS** overit `CZE | 433.775 MHz | BW 125.0 | SF12 | CR4/5 | 10 dBm`.
3. Prijmout znamy LoRa APRS paket a odeslat testovaci paket.
4. Zvolit `Vlastni`, nastavit napr. jinou nez provozni frekvenci a ulozit.
5. Pri prazdne TX fronte overit kratkou reinicializaci SX1278 a novou hodnotu na strance **LoRa APRS**.
6. Ulozit zmenu behem vysilani nebo pri cekajicim paketu; stranka ma zobrazit `CEKA` a zmenu aplikovat az po vyprazdneni fronty.
7. Behem cekani zvolit zpet aktualne aktivni profil a overit, ze `CEKA` zmizi bez zbytecne reinicializace radia.
8. Overit, ze se pri preladeni nevynuluji RX/TX/error pocitadla a nerestartuje GPS, displej, mapa ani Stopar.
9. Restartovat terminal a overit obnoveni vlastniho profilu z NVS.
10. Prepnout zpet na `CZE APRS`, ulozit a overit navrat na 433.775 MHz / BW125 / SF12 / CR4/5 / 10 dBm.
11. Po navratu na CZE znovu overit prijem a vysilani v ceske LoRa APRS siti.

## Poznamka k bezpecnosti provozu

Uzivatel odpovida za pouziti povolene frekvence, sirky pasma a vysilaciho vykonu.
Vychozi CZE profil zachovava dosavadni overene nastaveni projektu.
