# Rozsireny APRS parser

Firmware 1.4.0 uklada do modelu entity typ pozice, telemetrii, PHG, frekvencni informace a emergency stav. Frekvence se rozpoznava ve tvaru `145.650MHz`, tone ve tvaru `T088` a offset `+060` nebo `-060`. Klasicka telemetrie pouziva format `T#nnn,aaa,bbb,ccc,ddd,eee,xxxxxxxx`.

Komprimovane pozice, Mic-E, objekty a polozky jsou zpracovavany binarne bez prevodu neprintovatelnych Mic-E bytu na tecky.


## Analyza cesty od verze 2.7.6

Parser uchovava cast hlavicky mezi cilovou adresou a oddelovacem `:` jako cely APRS path. Prvky ukoncene `*` jsou povazovany za pouzite. Pocet pouzitych RF prvku urcuje pocet pruchodu pres DIGI a nejpravejsi pouzity prvek se zobrazuje jako posledni digipeater. `TCPIP`, `TCPXX`, `NOGATE`, `RFONLY` a konstrukce `qA` se nepocitaji jako RF digipeatery.

Paket bez pouziteho RF prvku se oznaci `DIRECT`, i kdyz obsahuje dosud nepouzitou pozadovanou cestu, napriklad `WIDE1-1,WIDE2-1`. U third-party paketu `}` se analyzuje vnitrni originalni ramec stejne jako pri urceni zdrojove stanice.
