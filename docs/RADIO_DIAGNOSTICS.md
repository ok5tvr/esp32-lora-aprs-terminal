# Diagnostika RSSI kanalu

Firmware 1.5.0 pridava samostatnou obrazovku **Diagnostika**.

## Princip

- frekvence je stejna jako aktivni LoRa APRS frekvence z `lora_profile.h`
- prvni automaticke mereni se provede priblizne 15 sekund po startu
- dalsi body se ukladaji po 300 sekundach
- jeden bod je prumer 8 okamzitych RSSI hodnot odebranych po 25 ms
- soucasne se uklada nejsilnejsi hodnota v danem mericim okne
- RAM historie obsahuje maximalne 20 bodu; po naplneni se nejstarsi odstrani
- historie se po restartu neobnovuje a nezapisuje se na SD kartu

## Ochrana prijmu a vysilani

Merici burst zacne pouze pokud je SX1278 v RX rezimu a centralni TX fronta je
prazdna. Pokud se mezitim rozbehne TX nebo se paket zaradi do fronty, rozdelane
mereni se zrusi a zopakuje za pet sekund. Cteni aktualniho RSSI je kratka SPI
operace a radio zustava v prijimacim rezimu.

## Interpretace

Graf nezobrazuje laboratorne kalibrovane ruseni ani procentualni obsazenost kanalu. Zobrazuje aktualni RSSI kanalu
v okamziku mereni. Do hodnoty se muze promítnout sum prijimace, cizi signal i
platny LoRa paket. Mene zaporna hodnota znamena silnejsi aktivitu, napriklad
-80 dBm je vyssi uroven nez -120 dBm.

Modra krivka zobrazuje prumer mericiho okna, oranzova jeho spicku.
