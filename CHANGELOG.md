# Changelog

## 2.7.2

- Fixed Arduino macro collision in `astronomy_screen.cpp`.
- Renamed local constants `PI` and `DEG_TO_RAD` to `ASTRO_PI` and `ASTRO_DEG_TO_RAD`.
- No functional change to astronomy calculations or the five-minute refresh interval.

## 2.7.1

- Added a dynamically drawn Moon phase disk to the Astronomy page; no bitmap assets are stored in Flash.
- Added current Sun altitude above or below the horizon.
- Sun altitude, Moon phase, illumination and Moon disk refresh every five minutes.
- Astronomy refreshes immediately when the local date, position source or position changes significantly.

## 2.7.0

- nova polozka hlavniho menu **Astronomie / Astronomy** vlozena za GPS prijimac
- offline vypocet vychodu a zapadu Slunce pro aktualni lokalni datum a GPS/vychozi polohu
- vypocet delky dne a podpora polarnich stavu bez vychodu nebo zapadu
- offline vypocet vychodu a zapadu Mesice s korekci horizontalni paralaxy
- zobrazeni osmi zakladnich fazi Mesice, osvetlene casti v procentech a priblizneho stari
- datum a mistni cas vychazeji z TimeService: RTC pri startu a automaticka synchronizace z GPS UTC
- vysledky se prepocitaji pouze pri zmene dne, zdroje polohy nebo presunu alespon o 5 km
- vsechny popisy jsou dostupne cesky i anglicky a prepnuti jazyka nevyzaduje restart
- doplnen hostitelsky test pro Plzen, polarni den, lunarni fazi a invalidni vstupy

## 2.6.1

- upraveno poradi hlavniho menu podle provozni priority
- nove poradi: LoRa APRS, Prijate stanice, Zpravy, Meteostanice, Mapa, Tracker, Stopar, DIGI / iGate, GPS prijimac, Diagnostika, Napajeni, Nastaveni
- polozka **Offline mapa / Offline map** prejmenovana na **Mapa / Map** v hlavnim menu i hlavicce mapove obrazovky
- cilove obrazovky a vsechny ostatni funkce zustavaji beze zmeny
- doplnen hostitelsky test presneho ceskeho a anglickeho poradi menu

## 2.6.0

- volba jazyka byla rozsirena z obrazovky Tracker na kompletni uzivatelske rozhrani
- nova centralni lokalizacni vrstva `App::Localization` s cestinou jako vychozim jazykem
- jazyk se nacita z NVS pred inicializaci displeje a sluzeb, aby byl spravny uz od startu
- prepnuti **Jazyk rozhrani / Interface language** se po ulozeni projevi okamzite bez restartu
- prelozeno hlavni menu, Nastaveni, Tracker, GPS, LoRa, zpravy, stanice, navigace, pocasi, mapa, Stopar, napajeni, diagnostika a DIGI/iGate
- prelozeny provozni stavy a chybova hlaseni generovana sluzbami radia, mapy, napajeni, Stopare, zprav a APRS-IS
- zachovana zpetna kompatibilita NVS: jazyk zustava ulozen pod klicem `trklang`
- APRS/NMEA/LoRa protokolova data, volaci znacky, jednotky a surove odpovedi serveru se neprekladaji
- prvni radek noveho TXT logu Stopare odpovida zvolenemu jazyku
- doplnen hostitelsky test lokalizace a syntakticka kontrola vsech obrazovek s `-Wall -Wextra -Werror`

## 2.5.0

- nova volba **Jazyk trackeru** na strance **Nastaveni**: `Cestina` nebo `English`
- vychozi jazyk zustava cestina; volba se uklada do NVS pod klicem `trklang`
- lokalizovany nazvy, napovedy a hodnoty vsech poli na obrazovce Trackeru
- lokalizovane nazvy APRS symbolu v rozbalovacim seznamu
- lokalizovany souhrn GPS, provozni stavy Trackeru, SmartBeaconu a rucniho BOOT beaconu
- lokalizovana potvrzeni ulozeni a chyby pri zapnuti trackeru, GPS, radia nebo Stopare
- volba jazyka nema vliv na APRS ramce, LoRa parametry, mapu ani ostatni obrazovky
- doplneny syntakticke hostitelske kontroly lokalizace, nastaveni a NVS rozhrani

## 2.4.0

- nova sekce **LoRa modul** na strance **Nastaveni**
- vychozi profil **CZE APRS** zachovava dosavadni 433.775 MHz, BW 125 kHz, SF12, CR 4/5 a TX 10 dBm
- volitelny profil **Vlastni** umoznuje frekvenci 410-525 MHz, BW 62.5/125/250/500 kHz, SF7-SF12, CR 4/5-4/8 a TX 2/5/10/14/17 dBm
- LoRa profil a parametry se ukladaji do NVS a nacitaji se pred inicializaci SX1278
- zmena za provozu ceka na dokonceni vysilani a vyprazdneni centralni TX fronty; potom se znovu inicializuje pouze LoRa modul
- pokud uzivatel pred aplikaci zmeny zvoli zpet aktualne aktivni parametry, cekajici preladeni se zrusi bez reinicializace
- RX/TX/error pocitadla se pri zmene profilu zachovavaji a GPS, displej, mapa, Stopar ani sitove sluzby se nerestartuji
- stranka **LoRa APRS** zobrazuje skutecne aktivni parametry a priznak `CEKA`, dokud nelze zmenu bezpecne aplikovat
- sync word 0x12, explicitni hlavicka, preambule 8 a vypnute RadioLib CRC zustavaji pevne kvuli kompatibilite paketoveho formatu
- rozsireny hostitelsky test NVS o vychozi CZE profil, validaci vlastniho profilu, persistenci a navrat k CZE hodnotam

## 2.3.0

- dvoustupnove setreni podsviceni pri provozu z baterie: 0-30 s nastaveny jas, od 30 s 15 %, po nastavenem timeoutu vypnuto
- s vychozim timeoutem 60 s odpovida prubeh presne 70 % -> 15 % -> 0 %
- jakakoli aktivita vrati ztlumeny displej na nastaveny jas; prvni dotyk po uplnem zhasnuti zustava pouze probouzecí
- USB-C nadale vynucuje 100 % jasu a vypina ztlumeni i automaticke zhasnuti
- nova `TimeService` pro onboard PCF85063 RTC na I2C adrese 0x51
- pri startu se cas nacte z RTC; platny GPS UTC cas automaticky synchronizuje systemovy cas i RTC
- RTC se z GPS znovu koriguje nejvyse jednou za sest hodin; neuspesny prvni zapis se opakuje po minute
- hlavni menu zobrazuje lokalni hodiny misto textu `LoRa`; zeleny cas znamena aktualni GPS synchronizaci, bily cas RTC
- automaticky prevod UTC na stredoevropsky cas CET/CEST vcetne evropskeho letniho casu
- nove hostitelske testy prechodu plny jas / ztlumeni / vypnuti a synchronizace RTC/GPS

## 2.2.0

- bateriovy usporny rezim displeje bez omezeni GPS, LoRa RX/TX, trackeru, DIGI, iGate, Stopare nebo mapy
- pri USB-C je podsviceni vzdy 100 % a automaticke vypnuti je zakazane
- pri provozu z baterie nastavitelny jas 10-100 %, vychozi 70 %
- nastavitelne vypnuti podsviceni: nikdy, 30 s, 60 s, 2 min nebo 5 min; vychozi 60 s
- jas a timeout se ukladaji do NVS na strance **Nastaveni**
- prvni dotyk po zhasnuti pouze probudi displej a je blokovan az do uvolneni prstu
- BOOT stisk pri zhasnutem displeji pouze probudi podsviceni a neodesle rucni beacon
- automaticke probuzeni a plny jas po pripojeni USB-C
- 20kHz osmibitove PWM podsviceni na GPIO25
- nova sluzba `DisplayPowerService` a hostitelsky test prechodu baterie/USB, timeoutu a probuzeni

## 2.1.0

- dotykove posouvani offline mapy tazenim prstu v libovolnem smeru
- plynuly nahled posunu: existujici mapa, APRS ikony, vlastni poloha a stopa se behem tahu pohybuji spolu
- skutecne precteni dlazdic se spusti az po uvolneni prstu, aby se SD karta nezatezovala pri kazdem dotykovem kroku
- sestipixelovy prah oddeluje kratky dotyk od posunu a ponechava prostor pro budouci vyber stanice
- po rucnim posunu se vypne automaticke sledovani GPS; stavovy radek zobrazi `MAN`
- tlacitko OK znovu zapne sledovani GPS nebo vychozi polohy a vycentruje mapu
- doplnena inverzni Web Mercator projekce a testy rucniho posunu, datove hranice a navratu na GPS

## 2.0.0

- nova polozka hlavniho menu **Offline mapa**
- standardni Web Mercator/XYZ projekce, zoom 3 az 18 a centrovani podle GPS nebo vychozi polohy
- mapove dlazdice `/MAP/<z>/<x>/<y>.rgb`, 256 x 256 little-endian RGB565
- progresivni nacitani nejvyse osmi radku dlazdice v jednom pruchodu smyckou
- mapovy framebuffer 480 x 202 v PSRAM; bez PSRAM se mapa bezpecne vypne
- modra vlastni poloha, plne APRS ikony stanic/objektu/polozek a cervene emergency oramovani
- oranžova stopa poslednich 64 bodu Stopare, zachovana i po ukonceni relace
- seda sachovnice a pocitadlo pri chybejici nebo poskozene dlazdici
- ovladani Nahoru/Dolu pro zoom a OK pro nove centrovani
- Python nastroj `tools/convert_map_tiles.py` pro prevod PNG/JPEG/WebP XYZ dlazdic
- nove testy mapove projekce, stavoveho automatu nacitani, konvertoru a zachovani stopy

## 1.5.0

- zobrazeni nazvu a verze firmware na strance **Nastaveni**
- nova samostatna polozka hlavniho menu **Diagnostika**
- periodicke mereni aktualniho RSSI kanalu 433,775 MHz po peti minutach
- jeden ulozeny bod je prumer osmi neblokujicich cteni RSSI po 25 ms; soucasne se uchovava spickova hodnota
- kruhova historie poslednich 20 mereni, tedy priblizne 100 minut provozu
- dvoukrivkovy LVGL graf: prumerne RSSI a nejsilnejsi hodnota v mericim okne
- mereni probiha pouze v RX rezimu s prazdnou TX frontou; pri vysilani nebo cekajicim TX se odlozi
- okamzite RSSI se cte primo pres RadioLib bez opusteni neblokujiciho prijmu

## 1.4.1

- fixed the PlatformIO build error in `RadioService`: `WeatherStore::ingest()` now receives `view_.lastPacketText` instead of the removed `view_.lastPacket` member
- retained the complete 1.4.0 extended APRS parser and weather-station detail functionality without behavioral changes

## 1.4.0

- rozsireny APRS parser o uchovani typu polohy: normalni, komprimovana a Mic-E
- rozsirene objekty a polozky vcetne kill reportu a frekvencnich objektu
- klasicka telemetrie `T#`, PHG, frekvence, tone, offset a emergency priznak
- emergency detekce z textoveho `!EMERGENCY!` a standardniho Mic-E stavu
- detail stanice zobrazuje format, telemetrii, PHG, frekvenci a emergency
- meteostanice maji vyber a samostatnou detailni obrazovku s poslednim TNC2 ramcem

## 1.3.1

- replaced category-only station icons with complete 94-entry primary and 94-entry alternate APRS symbol tables generated from the supplied reference chart
- made symbol rendering table-aware: `/` uses the primary table, `\` uses the alternate table, and digit/letter table identifiers use the alternate base plus an overlay
- retained compressed-position numeric overlay normalization from `a..j` to `0..9`
- corrected symbols that previously collapsed to the same generic image despite having different primary/alternate meanings
- changed the tracker generic selection from incorrect red-X `/.` to generic red-dot `//` and renamed it to `Obecny bod`
- kept compact alpha-only car, DIGI and iGate images for recolorable header status indicators
- added a reproducible APRS chart-to-RGB565 asset generator and lookup regression test
- retained the verified GPS GPIO4, RA-02 DIO0 GPIO2, TX queue, radio recovery and station navigation behavior

## 1.3.0

- added a fixed eight-entry central RF TX queue with deterministic priorities: ACK, outgoing message, DIGI, manual beacon, tracker and test
- kept FIFO ordering inside equal priorities and replaced stale queued scheduled-tracker frames with the newest position
- allowed higher-priority traffic to evict a lower-priority queued frame when the queue is full
- routed APRS message ACK/retries, digipeater frames, tracker/manual beacons and test packets through the same non-blocking scheduler
- expanded the LoRa page into a diagnostics view with APRS/decode counters, queue depth/max/drop/replacement counters, TX source and recovery statistics
- added automatic SX1278-only recovery after initialization failure, TX timeout or three consecutive RX read errors, rate-limited to five seconds
- preserved lifetime RX/TX/error counters across radio recovery
- added keyboard/button selection to the heard-entity list and an entity detail page with age, packet count, position, RSSI, SNR and last TNC2 frame
- added live distance and true-bearing navigation to a positioned station/object/item using GPS or the configured default reference
- retained the verified ATGM336H GPIO4, RA-02 DIO0 GPIO2 and 9600-baud hardware configuration

## 1.2.5

- moved ATGM336H GPS UART2 RX from GPIO17 to GPIO4 because GPIO17 is reserved by onboard PSRAM on this classic ESP32 board
- moved RA-02 / SX1278 DIO0 from GPIO4 to GPIO2; all other RA-02 connections remain unchanged
- restored the GPS serial speed to the ATGM336H default of 9600 baud, 8-N-1
- configured GPIO2 with an internal pulldown before RadioLib startup and documented the recommended external 10 kOhm pulldown for reset/upload reliability
- documented the requirement to keep onboard I2S audio disabled
- kept GPS sentence capture, tracker, Stopař, power telemetry and APRS processing unchanged

## 1.2.4

- changed the GPS UART2 receive speed from 9600 to 4800 baud
- kept the baud rate centralized in `AppConfig::GPS_BAUD_RATE`, so GPS startup logs and diagnostics show the same configured value
- clarified that the connected GPS receiver must already output NMEA at 4800 baud; this firmware change does not send a vendor-specific reconfiguration command to the module

## 1.2.3

- Fixed GPS diagnostic NMEA capture when UART contains embedded NUL/control bytes.
- NMEA sentences now finish on CR or LF and a lone `$` no longer replaces the previous line.
- The NMEA display uses a fixed two-line wrapping area.

## 1.2.2

- fixed the GPS diagnostics build by using the enabled Montserrat 14 font instead of unavailable Montserrat 12
- retained single-line ellipsis handling for the latest NMEA sentence
- removed deprecated mixed-enum bitwise warnings when creating navigation-button style selectors

## 1.2.1

- replaced the final GPS diagnostics counter row with the latest complete received NMEA sentence
- retained the complete sentence from `$` through its checksum while removing trailing CR/LF characters
- added a fixed 128-byte capture buffer and safe truncation for unusually long proprietary NMEA messages
- rendered the live sentence in a smaller single-line font with LVGL ellipsis protection
- added a host-side regression test for GNRMC/GPGGA sentence capture and sentence replacement

## 1.2.0

- added a read-only `PowerService` for the onboard AXP2101 over I2C
- added battery presence, state-of-charge, battery voltage, VBUS voltage,
  system voltage and internal PMIC-temperature readings
- added charging/discharging/standby/USB supply state and AXP2101 charger-phase decoding
- added display of configured charger current and target charge voltage without changing either setting
- added a permanent header summary with battery percentage, Czech decimal voltage and a battery, charge or USB symbol
- added green charging, blue USB and red critical-battery header states
- added a dedicated **Napajeni** main-menu page with live measurements and the latest detected power event
- added validation of PMIC percentage, voltage and temperature values before display
- added configurable two-second polling and 10 percent / 3.40 V critical thresholds
- kept GPS, LoRa receive, tracker and Stopar processing ahead of power polling in the main loop
- added XPowersLib 0.3.3 as an explicit PlatformIO dependency

## 1.1.0

- added the independent **Stopar** GPS route logger, enabled persistently from the Tracker page
- added a dedicated Stopar main-menu page with live state, active filename, point count, distance, duration and dropped-line counter
- added manual pause/resume using the touchscreen or the existing OK navigation action
- added autopause after 30 seconds without movement and automatic resume at 2 km/h or after an 8 metre displacement
- added semicolon-separated TXT route logs under `/STOPAR`, named from GPS UTC date/time
- added event records for start, stop, automatic pause/resume and manual pause/resume
- added a newest-first list of up to eight TXT logs with live file sizes
- added a compact save-state indicator to the main header
- added persistent NVS storage for the Stopar enable switch
- kept GPS, radio receive and APRS tracker handling ahead of SD writes in the main loop
- added a fixed 12-line RAM queue, one-line-per-loop SD writes and batched flushes to limit blocking
- added host-side mock-SD tests for session creation, autopause, resume, manual pause and log content

## 1.0.5

- Added a car APRS icon (`/>`) to the main header for tracker state.
- The tracker icon is green while scheduled tracking is active, amber while enabled but waiting for a usable position, and grey when disabled.
- Added a digipeater APRS icon (`/#`) that turns purple whenever RF digipeating is enabled.
- Added the LoRa iGate APRS icon (`L&`) using the same gateway asset and `L` overlay as station/object rendering.
- The iGate icon is green after a verified APRS-IS login, amber while enabled but not verified, and grey when disabled.
- Compacted all header indicator boxes to 30 px and repositioned them between `LoRa` and the Maidenhead locator.
- Retained message and new-station badges, the BOOT manual beacon, and `radio_.setCRC(false)`.

## 1.0.4

- Shortened the main-menu header title from `LoRa APRS terminal` to `LoRa`.
- Added three compact header indicators between the title and Maidenhead locator.
- GPS indicator uses grey for no traffic, orange for NMEA activity without a fix and green for a valid fix.
- Incoming APRS messages light the notification icon and increment an unread badge.
- Newly discovered stations, objects and items light the RF icon and increment a new-entity badge.
- Message notifications clear when the Messages page is opened; station notifications clear when Heard Stations is opened.
- Repeated copies of the same identified APRS message and repeated packets from an already-known station do not increment the badges.
- Added monotonic event counters to the fixed-size message and station stores and a native regression test.
- Retained the BOOT manual beacon and LoRa payload CRC disabled with `radio_.setCRC(false)`.

## 1.0.3

- Added debounced polling of the onboard BOOT button on GPIO0.
- A short BOOT press requests an immediate APRS position beacon.
- Manual beaconing works while periodic tracking is disabled and uses the saved tracker source, packet format and APRS symbol.
- GPS-source manual requests wait up to 15 seconds for a valid fix; default-position requests can transmit immediately.
- A busy radio delays the request instead of dropping it, while messages and digipeater traffic retain their existing scheduler priority.
- Manual transmission resets the normal tracker interval to avoid an immediate duplicate scheduled beacon.
- Added main-menu and tracker-screen feedback for BOOT beacon requests.
- RESET and PWR retain their original hardware functions.
- Retained LoRa payload CRC disabled with `radio_.setCRC(false)`.


## 1.0.2

- na uvodni obrazovku doplnen text `Vytvoril: OK5TVR`
- indikator spousteni byl posunut nize, aby se texty neprekryvaly

## 1.0.1

- opraveno prekryvani bileho nazvu a sede napovedy na strance DIGI / iGate
- svisle odsazeni radku je nyni explicitni a seda napoveda je posunuta o 4 px vyse
- popisky rozlisuji Digipeater RF -> RF a receive-only iGate RF -> APRS-IS
- doplnena kratka napoveda pro samostatne zapnuti jednotlivych sluzeb

## 1.0.0

- Added a dedicated touchscreen **DIGI / iGate** configuration and diagnostics page.
- Added persistent NVS enable switches for the digipeater and receive-only APRS-IS iGate.
- Added New-N style WIDE1-1 fill-in and traceable WIDE2-N operation with a configurable local maximum of one or two hops.
- Repeats only the first unused path component, marks the local callsign as used and decrements WIDE2-N when another hop remains.
- Added directed digipeating through an unused local callsign path and rejection of packets already repeated by this station.
- Added a 30-second path-independent duplicate cache and a randomized 120-420 ms retransmission delay.
- Added WiFi STA and APRS-IS configuration for server, port, passcode and optional server filter.
- Added verified APRS-IS login and RF-to-IS gating with the receive-only `qAO` construct.
- Honors `NOGATE`, `RFONLY`, `TCPIP`, `TCPXX`, existing q constructs and generic APRS queries.
- Unwraps eligible third-party packets before RF-to-IS gating and rejects Internet-origin third-party loops.
- Keeps Internet-to-RF gating disabled; this release is deliberately a receive-only iGate.
- Added fixed-capacity DIGI/iGate queues, counters and host-side tests without dynamic packet allocation.
- Discards APRS-IS queue entries older than 30 seconds after a network outage instead of uploading a stale burst.
- Reserves APRS-IS line space for the appended qAO path and enforces the 512-byte CR/LF line limit.
- Validates the RF/APRS-IS login callsign before allowing iGate startup.
- Retained the deployed LoRa profile with `radio_.setCRC(false)`.

## 0.9.3

- Added a touch dropdown for selecting the APRS symbol transmitted by the tracker.
- Added common choices for car, pedestrian, bicycle, motorcycle, QTH, boat, aircraft, balloon, weather station, generic station and LoRa iGate.
- Stored the selected tracker symbol persistently in ESP32 NVS.
- Applied the selected symbol to both normal and Base-91 compressed APRS positions.
- Added motorcycle rendering to the compact APRS icon mapper.
- Retained GPS diagnostics, messaging, SmartBeacon and `radio_.setCRC(false)`.

## 0.9.2

- Added live GPS/NMEA diagnostics: UART traffic, complete sentence and valid-checksum detection.
- Added last NMEA sentence type, packet ages, sentence counter and characters-per-second display.
- Added GPS UTC date/time, position, altitude, speed, course and cardinal direction display.
- Added six-character Maidenhead locator calculation, for example `JN69PS`.
- Added `GPS/DEF <locator>` status to the main LoRa APRS terminal header.
- Retained all APRS messages, tracker, weather, object/item and icon functions from 0.9.1.

## 0.9.1

- replace the two-character APRS symbol text in station/object/item and weather lists with compact graphical icons
- add 24 x 24 alpha-only LVGL assets for common mobile, fixed, weather, digi, gateway, balloon, aircraft, boat, bicycle and repeater symbols
- render APRS overlays on top of the alternate-table base icon
- explicitly support the LoRa iGate convention `L&` as a gateway diamond with an `L` overlay
- preserve the original two-character APRS code as a fallback for unsupported symbols
- keep image assets linked in firmware Flash and recolor them at runtime
- retain LoRa payload CRC disabled with `radio_.setCRC(false)`

## 0.9.0

- add APRS directed-message parsing and encoding with the fixed nine-character addressee field
- add a Messages screen with a 20-entry incoming/outgoing RAM history
- add a two-step touchscreen composer for recipient and message text
- generate three-digit message identifiers and request acknowledgements for outgoing messages
- automatically transmit `ack` responses for directed messages addressed to the configured callsign
- recognize incoming `ack` and `rej` responses and update delivery state
- retry unacknowledged messages up to five times while keeping LoRa reception active in the background
- merge duplicate identified incoming messages while acknowledging repeated copies
- accept group messages addressed to `ALL`, `QST` and `CQ` without acknowledging them
- support one level of third-party encapsulation for APRS messages
- retain LoRa payload CRC disabled with `radio_.setCRC(false)`

## 0.8.1

- fixed build errors caused by references to unavailable LVGL built-in fonts
- replaced Montserrat 12/13/20 usages with enabled Montserrat 14/18 fonts
- added explicit LVGL style-selector casts to silence GCC 14 enum warnings

## 0.8.0

- add distance and initial-bearing calculation from the current reference position to heard APRS entities and weather stations
- prefer a fresh GPS fix as the reference position and fall back to the configured default latitude/longitude
- add a generic NMEA GPS service on UART2 RX GPIO17 at 9600 baud using TinyGPSPlus
- add a live GPS status screen with receiver detection, fix, position, altitude, speed, course, satellites and HDOP
- add a persistent APRS tracker screen and NVS configuration
- select GPS or default coordinates as the tracker position source
- select normal uncompressed or Base-91 compressed APRS position reports
- select fixed-interval beaconing or GPS-based SmartBeacon scheduling
- prevent SmartBeacon selection with the static default position
- require a detected GPS receiver before enabling GPS-source tracking, then wait for a valid fix before transmitting
- continue LoRa reception, APRS parsing, GPS reading and tracker scheduling in the background on every screen
- re-arm the tracker whenever saved tracker settings change
- add APRS position-frame encoder tests and host-side distance/bearing tests
- retain LoRa payload CRC disabled with `radio_.setCRC(false)`

## 0.7.0

- add a dedicated APRS weather-station screen with five unique source callsigns
- move an already-known weather station to the top and overwrite the oldest entry when the sixth station is heard
- decode common APRS weather fields: temperature, humidity, pressure, wind direction, wind speed, gust, rainfall and solar radiation
- support complete weather reports with `ddd/sss`, positionless weather fields `cddd`/`sddd`, and weather data following station, object or item positions
- convert displayed values to degrees Celsius, percent, hPa, km/h and millimetres
- keep LoRa reception, APRS decoding and both station stores active on every screen
- add persistent NVS settings for callsign and default latitude/longitude
- use the saved callsign for the test transmission
- add a modal LVGL touch keyboard for callsign and numeric coordinate editing
- retain `radio_.setCRC(false)`; no `setCRC(true)` call is present

## 0.6.0

- add binary-safe Mic-E decoding from the TNC2 destination and information fields
- preserve non-printable Mic-E bytes for parsing while keeping the UI text printable
- decode Mic-E latitude, longitude and the symbol table/code
- decode live and killed APRS objects with fixed nine-character names
- decode live and killed APRS items with variable three-to-nine-character names
- store objects and items as independent entities instead of assigning their position to the transmitting station
- preserve the original transmitting callsign as the object/item owner
- remove killed objects/items from the visible list
- keep object/item names case-sensitive as required by APRS
- confirm and retain `radio_.setCRC(false)` for LoRa APRS compatibility


## 0.5.0

- add a fixed-memory list of the 15 most recently heard unique APRS stations
- keep the original source callsign, including SSID, from the TNC2 frame
- decode APRS symbol table/code as `/x`, `\x` or the received overlay code
- decode standard uncompressed and compressed GPS positions for `!`, `=`, `/` and `@` packets
- preserve a station's last known position when a later status packet has no position
- move an already-known station to the top when it is heard again
- replace the oldest station when a sixteenth unique callsign is heard
- add a touch-scrollable Heard Stations screen
- keep LoRa reception and station collection active in the background on every screen
- use the original inner source callsign for one-level third-party (`}`) frames
- disable LoRa payload CRC for the deployed LoRa APRS profile
- replace the obsolete CRC counter with a general RX error counter

## 0.4.3

- fixed LoadProhibited crash in the LoRa screen caused by LVGL float formatting
- use the standard C formatter for LVGL formatted labels
- format LoRa parameters and signal values into bounded local buffers
- guard LoRa label pointers before periodic updates
- avoid clearing a RadioLib interrupt callback before one has been installed
- remove the harmless GPIO ISR service warning during the first RX startup

## 0.4.2

- defer all navigation until after the LVGL event callback has returned
- prevent deletion of the active navigation button from its own callback
- add a short navigation lock to prevent click-through into a newly created screen
- use GPIO4 for RA-02 DIO0; onboard audio must remain disabled
- log the previous ESP32 reset reason and free memory at startup

## 0.4.1

- moved LVGL dynamic allocations to PSRAM with internal-RAM fallback
- replaced the large static LVGL draw buffer with a runtime DMA-capable buffer
- reduced the draw buffer from 30 to 12 lines
- removed the 80 kB static LVGL heap from `.dram0.bss`
- added startup memory diagnostics
- fixed classic ESP32 linker overflow in `dram0_0_seg`

## 0.4.0

- changed target from ESP32-S3 to classic ESP32-D0WDR2-V3
- added custom PlatformIO board definition for 16 MB Flash and 2 MB PSRAM
- corrected ST7796, FT6336, TCA9554 and I2C pin mapping
- placed LCD + microSD on VSPI and RA-02 on independent HSPI
- added onboard microSD initialization
- corrected touch transformation for landscape rotation 1
- reduced LVGL internal memory use for the classic ESP32

## 0.3.0

- initial modular project with LVGL and RA-02 support
