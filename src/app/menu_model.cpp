#include "app/menu_model.h"

#include "app/localization.h"

namespace App {
namespace MenuModel {
namespace {

constexpr MenuItem ITEMS_CZECH[ITEM_COUNT] = {
    {"LoRa APRS", "Stav radia, TX fronty a automaticke obnovy.", ScreenId::LoRaStatus},
    {"Prijate stanice", "Vyber, detail a navigace k poslednim APRS entitam.", ScreenId::Stations},
    {"Zpravy", "Prijem, odesilani a potvrzovani APRS zprav.", ScreenId::Messages},
    {"Meteostanice", "Poslednich 5 unikatnich APRS meteostanic.", ScreenId::Weather},
    {"Mapa", "Mapove dlazdice z SD, vlastni stopa a APRS stanice.", ScreenId::Map},
    {"Tracker", "GPS/default pozice, cesta, komentar a SmartBeacon profily.", ScreenId::Tracker},
    {"Beacon", "Jednorazovy APRS beacon z GPS nebo vychozi polohy.", ScreenId::Beacon},
    {"Stopar", "Automaticky zaznam GPS trasy a ulozene TXT logy.", ScreenId::Trail},
    {"DIGI / iGate", "WIDE1/WIDE2 digipeater a jednosmerna APRS-IS iGate.", ScreenId::DigiIgate},
    {"GPS prijimac", "NMEA diagnostika, poloha, rychlost, smer a lokator.", ScreenId::GpsStatus},
    {"Astronomie", "Vychod a zapad Slunce a Mesice, faze Mesice.", ScreenId::Astronomy},
    {"Diagnostika", "Historie aktualniho RSSI na LoRa frekvenci.", ScreenId::Diagnostics},
    {"Napajeni", "AXP2101: baterie, USB-C, nabijeni a dlouhodoba historie.", ScreenId::Power},
    {"Nastaveni", "CALL, poloha, displej, jazyk a LoRa profil.", ScreenId::Settings}
};

constexpr MenuItem ITEMS_ENGLISH[ITEM_COUNT] = {
    {"LoRa APRS", "Radio status, TX queue and automatic recovery.", ScreenId::LoRaStatus},
    {"Received stations", "Select, inspect and navigate to recent APRS entities.", ScreenId::Stations},
    {"Messages", "Receive, send and acknowledge APRS messages.", ScreenId::Messages},
    {"Weather stations", "The last 5 unique APRS weather stations.", ScreenId::Weather},
    {"Map", "SD map tiles, own trail and APRS stations.", ScreenId::Map},
    {"Tracker", "GPS/default position, path, comment and SmartBeacon profiles.", ScreenId::Tracker},
    {"Beacon", "One-shot APRS beacon from GPS or the default position.", ScreenId::Beacon},
    {"Trail logger", "Automatic GPS route recording and saved TXT logs.", ScreenId::Trail},
    {"DIGI / iGate", "WIDE1/WIDE2 digipeater and one-way APRS-IS iGate.", ScreenId::DigiIgate},
    {"GPS receiver", "NMEA diagnostics, position, speed, course and locator.", ScreenId::GpsStatus},
    {"Astronomy", "Sunrise, sunset, moonrise, moonset and Moon phase.", ScreenId::Astronomy},
    {"Diagnostics", "Current RSSI history on the LoRa frequency.", ScreenId::Diagnostics},
    {"Power", "AXP2101 battery, USB-C, charging and long-term history.", ScreenId::Power},
    {"Settings", "Callsign, position, display, language and LoRa profile.", ScreenId::Settings}
};

}  // namespace

std::size_t count() {
    return ITEM_COUNT;
}

const MenuItem& item(std::size_t index) {
    const MenuItem* items = Localization::isEnglish() ? ITEMS_ENGLISH : ITEMS_CZECH;
    return items[index % ITEM_COUNT];
}

}  // namespace MenuModel
}  // namespace App
