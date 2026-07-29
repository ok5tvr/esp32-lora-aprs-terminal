#include "app/menu_model.h"

namespace App {
namespace MenuModel {
namespace {

constexpr MenuItem ITEMS[ITEM_COUNT] = {
    {"LoRa APRS", "Stav radia, prijem a testovaci vysilani.", ScreenId::LoRaStatus},
    {"Zpravy", "Prijem, odesilani a potvrzovani APRS zprav.", ScreenId::Messages},
    {"GPS prijimac", "NMEA diagnostika, poloha, rychlost, smer a lokator.", ScreenId::GpsStatus},
    {"Prijate stanice", "Poslednich 15 unikatnich slysenych APRS entit.", ScreenId::Stations},
    {"Meteostanice", "Poslednich 5 unikatnich APRS meteostanic.", ScreenId::Weather},
    {"Tracker", "GPS/default pozice, format a SmartBeacon.", ScreenId::Tracker},
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
