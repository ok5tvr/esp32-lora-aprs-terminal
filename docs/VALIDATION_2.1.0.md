# Validace 2.1.0 - dotykove posouvani mapy

## Automaticke a hostitelske kontroly

- dopredna a inverzni Web Mercator projekce vcetne kruhoveho prechodu zemepisne delky
- rucni posun o 64 pixelu a kontrola odpovidajiciho posunu referencni GPS polohy na obrazovce
- potvrzeni, ze nova GPS revize neprepise stred mapy v rezimu `MAN`
- potvrzeni, ze OK/recenter vrati rezim sledovani a umisti aktualni referenci doprostred
- prisna syntaxe `MapProjection`, `MapService`, `MapScreen`, `ScreenManager` a `AppController` s `-Wall -Wextra -Werror`
- zachovani progresivniho cteni dlazdic, zoomu a ukonceni cteni pri opusteni obrazovky

## PlatformIO build

```powershell
pio run -e waveshare-esp32-release
```

## Funkcni test na desce

1. Otevrit **Offline mapa** a pockat na dokonceni nacteni dlazdic.
2. Tahnout mapu doprava; podklad, stopa, vlastni bod a APRS ikony se musi pohybovat spolecne.
3. Uvolnit prst; stavovy radek musi prejit na `MAN` a nacist novy vyrez.
4. Behem rucniho rezimu zmenit GPS polohu; stred mapy se nesmi automaticky vratit.
5. Stisknout OK; stav se musi vratit na `GPS` nebo `DEF` a aktualni poloha se vycentruje.
6. Overit tah nahoru, dolu, vlevo a sikmo, vcetne tahu pres okraj mapoveho vyrezu.
7. Kratce klepnout bez pohybu; mapa se nesmi posunout.
8. Behem nacitani noveho vyrezu prijmout a odeslat LoRa paket; RX/TX musi zustat funkcni.
9. Overit zoom v rucnim rezimu; geograficky stred se pri zoomu nesmi zmenit.

## Poznamky

- dotykovy tah se potvrdi az po prekroceni prahu `MAP_TOUCH_DRAG_THRESHOLD_PIXELS`
- nacitani noveho vyrezu zacina az po uvolneni prstu
- OK je jediny prikaz, ktery rucni rezim zpetne prepne na automaticke sledovani
