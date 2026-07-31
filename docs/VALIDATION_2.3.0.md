# Validace 2.3.0 - ztlumeni displeje a RTC/GPS cas

## Automaticke testy

- baterie, jas 70 %, timeout 60 s: 0-29,999 s = 70 %, 30-59,999 s = 15 %, od 60 s = 0 %
- aktivita ve ztlumenem stavu vrati nastaveny jas
- prvni aktivita po vypnuti probudi displej a vrati nastaveny jas
- USB-C vynuti 100 % a zakaze ztlumeni i vypnuti
- RTC zimni cas: 12:00 UTC se zobrazi 13:00 CET
- GPS letni cas: 08:18 UTC se zobrazi 10:18 CEST
- platny GPS cas prepise datum a cas v simulovanych registrech PCF85063
- neplatny RTC obsah se pri cekani na GPS neopakuje v kazdem pruchodu smyckou; dalsi pokus je nejdrive po 60 s
- nove sluzby prochazeji prekladem s `-Wall -Wextra -Werror` proti hostitelskym stubum

## Test na zarizeni

1. Pri USB-C overit 100 % jasu bez ztlumeni po vice nez 60 s.
2. Odpojit USB-C a nastavit 70 % / 60 s.
3. Po 30 s overit ztlumeni na 15 %.
4. Po 60 s overit uplne zhasnuti podsviceni.
5. Dotyk ve ztlumenem stavu musi ihned vratit 70 % a soucasne ovladat UI.
6. Prvni dotyk po uplnem zhasnuti musi pouze probudit displej.
7. Na hlavnim menu overit hodiny misto textu `LoRa`.
8. Bez GPS overit bily cas z RTC; po prijeti platneho NMEA data/casu zeleny cas.
9. Restartovat bez GPS a overit, ze RTC zachovalo cas.
10. Overit prechod CET/CEST na znamem datu nebo simulovanym GPS vstupem.
11. Behem ztlumeni a zhasnuti overit nepretrzity LoRa RX/TX, tracker, DIGI, iGate a Stopar.
