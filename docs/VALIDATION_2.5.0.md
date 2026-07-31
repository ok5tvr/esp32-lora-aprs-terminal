# Validace 2.5.0 - cestina a anglictina Trackeru

## Rozsah

Verze pridava volbu `Jazyk trackeru` do stranky Nastaveni. Vychozi hodnota je
`Cestina`; alternativou je `English`. Volba se uklada do NVS a nema vliv na
format APRS ramce ani konfiguraci LoRa.

## Kontrolni scenare

1. Vymazte NVS nebo nahrajte firmware na ciste zarizeni. V Nastaveni musi byt
   zvolena `Cestina` a Tracker musi zobrazovat ceske popisy.
2. Zvolte `English`, stisknete `Ulozit`, opustte Nastaveni a otevřete Tracker.
   Nazvy poli, napovedy, ON/OFF hodnoty, GPS souhrn, symboly a stavove texty musi
   byt anglicky.
3. Restartujte terminal. Anglictina musi zustat zachovana.
4. Zvolte zpet `Cestina`, ulozte a znovu otevřete Tracker. Vsechny popisy se musi
   vratit do cestiny.
5. V anglictine zkuste zapnout Tracker bez GPS, bez inicializovaneho radia a
   Stopar bez SD karty. Chybove texty musi byt anglicky.
6. Overte rucni BOOT beacon, cekani na GPS fix, pevny interval a SmartBeacon.
   Provozni stav v Trackeru musi odpovidat zvolenemu jazyku.
7. V obou jazycich projdete vsech 11 APRS symbolu. Poradi a APRS table/code musi
   zustat shodne; meni se pouze zobrazene nazvy.
8. Overte, ze LoRa profil, CRC, GPS, mapa, DIGI/iGate, zpravy a obsah odeslaneho
   APRS paketu zustaly beze zmeny.

## Hostitelske kontroly

Zmenene obrazovky byly syntakticky overeny pomoci `clang++ -std=c++17` s LVGL
stubem. `settings_service.cpp`, lokalizacni tabulky a TrackerService byly rovnez
zkontrolovany s `-Wall -Wextra -Werror`. Plny PlatformIO build je nutne spustit
lokalne pro cilovou desku.
