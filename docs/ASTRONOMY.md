# Astronomie ve verzi 2.7.0

Stranka **Astronomie / Astronomy** pracuje zcela offline. Nepotrebuje Wi-Fi ani
internetovou sluzbu.

## Vstupy

- lokalni datum a cas ze `TimeService`
- pri startu je cas nacten z PCF85063 RTC
- platny GPS UTC cas synchronizuje RTC i bez platneho pozicniho fixu
- poloha se bere z GPS fixu; bez fixu se pouzije vychozi poloha z Nastaveni
- zobrazene casy pouzivaji stejne CET/CEST pravidlo jako hodiny terminalu

## Vystupy

- vychod a zapad Slunce
- delka dne
- vychod a zapad Mesice
- faze Mesice v osmi kategoriich
- osvetlena cast Mesice v procentech
- priblizne stari Mesice ve dnech

## Vypocet

Slunce a Mesic se pocitaji pomoci nizkopresnostnich geocentrickych efemerid
vhodnych pro terenni informacni displej. Vychody a zapady se hledaji jako
prechody vysky telesa pres standardni horizont a cas se zpresni pulenim
intervalu. Pro Mesic se pouziva korekce zavisla na horizontalni paralaxe.

Vysledky jsou orientacni. Mistni horizont, teren, budovy, refrakce a aktualni
atmosfericke podminky mohou skutecny pozorovany cas posunout o nekolik minut.

## Energetika

Vypocet neprobiha v kazdem pruchodu hlavni smyckou. Opakuje se pouze pri:

- zmene lokalniho data
- prechodu mezi GPS a vychozi polohou
- presunu alespon o 5 km

LoRa RX/TX, Tracker, DIGI/iGate, Stopar a mapa nejsou vypoctem pozastaveny.
