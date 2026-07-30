#include "app/menu_model.h"

namespace App {
namespace MenuModel {
namespace {

constexpr MenuItem ITEMS[ITEM_COUNT] = {
    {"LoRa APRS", "Stav radia, TX fronty a automaticke obnovy.", ScreenId::LoRaStatus},
    {"Diagnostika", "Historie aktualniho RSSI na LoRa frekvenci.", ScreenId::Diagnostics},
    {"Zpravy", "Prijem, odesilani a potvrzovani APRS zprav.", ScreenId::Messages},
    {"GPS prijimac", "NMEA diagnostika, poloha, rychlost, smer a lokator.", ScreenId::GpsStatus},
    {"Offline mapa", "Mapove dlazdice z SD, vlastni stopa a APRS stanice.", ScreenId::Map},
    {"Prijate stanice", "Vyber, detail a navigace k poslednim APRS entitam.", ScreenId::Stations},
    {"Meteostanice", "Poslednich 5 unikatnich APRS meteostanic.", ScreenId::Weather},
    {"Tracker", "GPS/default pozice, format a SmartBeacon.", ScreenId::Tracker},
    {"Stopar", "Automaticky zaznam GPS trasy a ulozene TXT logy.", ScreenId::Trail},
    {"Napajeni", "AXP2101: baterie, USB-C, nabijeni a teplota PMIC.", ScreenId::Power},
    {"DIGI / iGate", "WIDE1/WIDE2 digipeater a jednosmerna APRS-IS iGate.", ScreenId::DigiIgate},
    {"Nastaveni", "Editace CALL a vychozi GPS polohy.", ScreenId::Settings}
};

}  // namespace

std::size_t count() {
    return ITEM_COUNT;
}

const MenuItem& item(std::size_t index) {
    return ITEMS[index % ITEM_COUNT];
}

}  // namespace MenuModel
}  // namespace App
