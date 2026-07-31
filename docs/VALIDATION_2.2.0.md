# Validace 2.2.0 - uspora baterie displejem

## Automaticke testy

- baterie: aplikace nastaveneho jasu
- timeout 60 s: pred limitem displej sviti, na limitu se vypne
- prvni aktivita po vypnuti probudi podsviceni
- USB-C: vynuceni 100 % a zakaz automatickeho vypnuti
- timeout `Nikdy`: podsviceni zustava aktivni
- zmena jasu v NVS se aplikuje bez restartu

## Test na zarizeni

1. Pripojit USB-C a overit 100 % jasu i po vice nez peti minutach.
2. Odpojit USB-C, nastavit 70 % a 60 s, ulozit a overit snizeni jasu.
3. Po 60 s bez dotyku overit zhasnuti podsviceni.
4. Dotknout se ovladaciho prvku; prvni dotyk ma pouze probudit displej.
5. Stisknout BOOT pri zhasnutem displeji; displej se probudi bez TX beaconu.
6. Pri aktivnim trackeru, DIGI a Stopaři overit, ze zhasnuti nezastavi sluzby.
7. Pri zhasnutem displeji pripojit USB-C; do dalsiho PMIC pollu se ma displej probudit na 100 %.
8. Overit vsechny timeouty: Nikdy, 30 s, 60 s, 2 min, 5 min.
