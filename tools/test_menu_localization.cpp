#include <cassert>
#include <cstring>
#include <iostream>

#include "app/localization.h"
#include "app/menu_model.h"

namespace {

void verifyMenu(
    App::UiLanguage language,
    const char* const expectedTitles[App::MenuModel::ITEM_COUNT]) {

    App::Localization::setLanguage(language);
    assert(App::MenuModel::count() == App::MenuModel::ITEM_COUNT);

    const App::ScreenId expectedTargets[App::MenuModel::ITEM_COUNT] = {
        App::ScreenId::LoRaStatus,
        App::ScreenId::Stations,
        App::ScreenId::Messages,
        App::ScreenId::Weather,
        App::ScreenId::Map,
        App::ScreenId::Tracker,
        App::ScreenId::Trail,
        App::ScreenId::DigiIgate,
        App::ScreenId::GpsStatus,
        App::ScreenId::Astronomy,
        App::ScreenId::Diagnostics,
        App::ScreenId::Power,
        App::ScreenId::Settings
    };

    for (std::size_t index = 0; index < App::MenuModel::ITEM_COUNT; ++index) {
        const App::MenuModel::MenuItem& item = App::MenuModel::item(index);
        assert(std::strcmp(item.title, expectedTitles[index]) == 0);
        assert(item.target == expectedTargets[index]);
    }

    assert(App::MenuModel::item(App::MenuModel::ITEM_COUNT).target ==
           App::MenuModel::item(0).target);
}

}  // namespace

int main() {
    const char* const czechTitles[App::MenuModel::ITEM_COUNT] = {
        "LoRa APRS",
        "Prijate stanice",
        "Zpravy",
        "Meteostanice",
        "Mapa",
        "Tracker",
        "Stopar",
        "DIGI / iGate",
        "GPS prijimac",
        "Astronomie",
        "Diagnostika",
        "Napajeni",
        "Nastaveni"
    };

    const char* const englishTitles[App::MenuModel::ITEM_COUNT] = {
        "LoRa APRS",
        "Received stations",
        "Messages",
        "Weather stations",
        "Map",
        "Tracker",
        "Trail logger",
        "DIGI / iGate",
        "GPS receiver",
        "Astronomy",
        "Diagnostics",
        "Power",
        "Settings"
    };

    verifyMenu(App::UiLanguage::Czech, czechTitles);
    verifyMenu(App::UiLanguage::English, englishTitles);

    std::cout << "menu localization and ordering tests passed\n";
    return 0;
}
