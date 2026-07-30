# Rozsireny APRS parser

Firmware 1.4.0 uklada do modelu entity typ pozice, telemetrii, PHG, frekvencni informace a emergency stav. Frekvence se rozpoznava ve tvaru `145.650MHz`, tone ve tvaru `T088` a offset `+060` nebo `-060`. Klasicka telemetrie pouziva format `T#nnn,aaa,bbb,ccc,ddd,eee,xxxxxxxx`.

Komprimovane pozice, Mic-E, objekty a polozky jsou zpracovavany binarne bez prevodu neprintovatelnych Mic-E bytu na tecky.
