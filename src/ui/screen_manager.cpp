#include "ui/screen_manager.h"

#include <Arduino.h>
#include <cstring>

#include "app/menu_model.h"
#include "app/localization.h"
#include "app_config.h"
#include "ui/screens/astronomy_screen.h"
#include "ui/screens/diagnostics_screen.h"
#include "ui/screens/digi_igate_screen.h"
#include "ui/screens/gps_screen.h"
#include "ui/screens/lora_screen.h"
#include "ui/screens/map_screen.h"
#include "ui/screens/menu_screen.h"
#include "ui/screens/messages_screen.h"
#include "ui/screens/placeholder_screen.h"
#include "ui/screens/power_screen.h"
#include "ui/screens/settings_screen.h"
#include "ui/screens/splash_screen.h"
#include "ui/screens/stations_screen.h"
#include "ui/screens/station_detail_screen.h"
#include "ui/screens/station_navigation_screen.h"
#include "ui/screens/trail_screen.h"
#include "ui/screens/tracker_screen.h"
#include "ui/screens/weather_screen.h"
#include "ui/screens/weather_detail_screen.h"
#include "ui/ui_components.h"
#include "ui/ui_styles.h"

namespace Ui {
namespace {

void addNotificationCount(std::uint8_t& count, std::uint32_t delta) {
    const std::uint32_t sum = static_cast<std::uint32_t>(count) + delta;
    count = static_cast<std::uint8_t>(sum > 99U ? 99U : sum);
}

MenuScreen::IndicatorState makeMenuIndicators(
    const Services::GpsService::ViewState& gps,
    const Services::TrackerService::ViewState& tracker,
    const Services::TrailService::ViewState& trail,
    const Services::DigiIgateService::ViewState& digiIgate,
    std::uint8_t unreadMessages,
    std::uint8_t newStations) {

    MenuScreen::IndicatorState state;
    state.gpsSerialTraffic = gps.serialTrafficDetected;
    state.gpsNmeaPacket = gps.nmeaPacketDetected;
    state.gpsReceiverDetected = gps.receiverDetected;
    state.gpsFix = gps.hasFix;
    state.trackerConfigured = tracker.configuredEnabled;
    state.trackerActive = tracker.active;
    state.trailConfigured = trail.configuredEnabled;
    state.trailRecording = trail.state == Services::TrailService::State::Recording;
    state.trailPaused = trail.state == Services::TrailService::State::AutoPaused ||
        trail.state == Services::TrailService::State::ManualPaused ||
        trail.state == Services::TrailService::State::WaitingForGps ||
        trail.state == Services::TrailService::State::WaitingForSd;
    state.trailError = trail.state == Services::TrailService::State::Error;
    state.digiEnabled = digiIgate.digiEnabled;
    state.igateEnabled = digiIgate.igateEnabled;
    state.igateVerified = digiIgate.aprsIsVerified;
    state.unreadMessages = unreadMessages;
    state.newStations = newStations;
    return state;
}

}  // namespace

void ScreenManager::begin(
    App::CommandHandler commandHandler,
    void* commandContext,
    App::MessageSendHandler messageSendHandler,
    void* messageSendContext,
    App::SettingsSaveHandler settingsSaveHandler,
    void* settingsSaveContext,
    App::DigiIgateSettingsSaveHandler digiIgateSaveHandler,
    void* digiIgateSaveContext,
    App::TrackerSettingsSaveHandler trackerSaveHandler,
    void* trackerSaveContext,
    App::MapPanHandler mapPanHandler,
    void* mapPanContext) {

    commandHandler_ = commandHandler;
    commandContext_ = commandContext;
    messageSendHandler_ = messageSendHandler;
    messageSendContext_ = messageSendContext;
    settingsSaveHandler_ = settingsSaveHandler;
    settingsSaveContext_ = settingsSaveContext;
    digiIgateSaveHandler_ = digiIgateSaveHandler;
    digiIgateSaveContext_ = digiIgateSaveContext;
    trackerSaveHandler_ = trackerSaveHandler;
    trackerSaveContext_ = trackerSaveContext;
    MapScreen::setPanHandler(mapPanHandler, mapPanContext);
    observedMessageEvents_ = 0;
    observedStationEvents_ = 0;
    unreadMessageCount_ = 0;
    newStationCount_ = 0;
    renderedLanguage_ = App::Localization::language();
    Styles::begin();
    currentScreen_ = App::ScreenId::Splash;
    SplashScreen::create();
}

void ScreenManager::update(
    std::uint32_t now,
    const Services::RadioService::ViewState& radioState,
    const Services::MessageStore::ViewState& messageState,
    const Services::StationStore::ViewState& stationState,
    const Services::WeatherStore::ViewState& weatherState,
    const Services::GpsService::ViewState& gpsState,
    const Services::TrackerService::ViewState& trackerState,
    const Services::TrailService::ViewState& trailState,
    const Services::PowerService::ViewState& powerState,
    const Services::TimeService::ViewState& timeState,
    const Services::AstronomyService::ViewState& astronomyState,
    const Services::DigiIgateService::ViewState& digiIgateState,
    const Services::PositionReference& reference,
    const Services::MapService::ViewState& mapState,
    const Services::SettingsService::ViewState& settingsState) {

    settingsState_ = settingsState;
    radioState_ = &radioState;
    gpsState_ = gpsState;
    stationState_ = &stationState;
    weatherState_ = &weatherState;
    refreshSelectedStation();
    if (selectedWeatherValid_) {
        for (std::size_t index = 0; index < weatherState.count; ++index) {
            if (std::strcmp(weatherState.stations[index].callsign, selectedWeather_.callsign) == 0) {
                selectedWeather_ = weatherState.stations[index];
                break;
            }
        }
    }
    trackerState_ = trackerState;
    trailState_ = trailState;
    powerState_ = powerState;
    timeState_ = timeState;
    astronomyState_ = astronomyState;
    digiIgateState_ = digiIgateState;
    referenceState_ = reference;
    mapState_ = &mapState;

    const bool languageChanged = renderedLanguage_ != settingsState.uiLanguage;
    if (languageChanged) {
        renderedLanguage_ = settingsState.uiLanguage;
        App::Localization::setLanguage(renderedLanguage_);
        show(currentScreen_);
    }

    const std::uint32_t messageDelta =
        messageState.receivedMessageEvents - observedMessageEvents_;
    observedMessageEvents_ = messageState.receivedMessageEvents;
    if (messageDelta > 0U) {
        addNotificationCount(unreadMessageCount_, messageDelta);
    }

    const std::uint32_t stationDelta =
        stationState.discoveredEntities - observedStationEvents_;
    observedStationEvents_ = stationState.discoveredEntities;
    if (stationDelta > 0U) {
        addNotificationCount(newStationCount_, stationDelta);
    }

    if (currentScreen_ == App::ScreenId::Messages) {
        unreadMessageCount_ = 0;
    } else if (currentScreen_ == App::ScreenId::Stations) {
        newStationCount_ = 0;
    }

    if (currentScreen_ == App::ScreenId::Settings) {
        SettingsScreen::processPending();
    } else if (currentScreen_ == App::ScreenId::DigiIgate) {
        DigiIgateScreen::processPending();
    } else if (currentScreen_ == App::ScreenId::Messages) {
        MessagesScreen::processPending();
    }

    // Navigation is deliberately processed here, after lv_timer_handler() has
    // returned. Deleting the active button/screen from inside its LVGL event
    // callback can invalidate the object currently dispatching the event.
    processPendingNavigation();

    if (currentScreen_ == App::ScreenId::Splash && SplashScreen::finished(now)) {
        showMainMenu();
        return;
    }

    if (now - lastRefreshAt_ < AppConfig::UI_REFRESH_INTERVAL_MS) {
        return;
    }
    lastRefreshAt_ = now;
    updateHeaderPower(powerState);
    updateHeaderTime(timeState);

    if (currentScreen_ == App::ScreenId::MainMenu) {
        MenuScreen::update(
            reference,
            makeMenuIndicators(
                gpsState,
                trackerState,
                trailState,
                digiIgateState,
                unreadMessageCount_,
                newStationCount_));
    } else if (currentScreen_ == App::ScreenId::LoRaStatus) {
        LoRaScreen::update(radioState);
    } else if (currentScreen_ == App::ScreenId::Diagnostics) {
        DiagnosticsScreen::update(radioState, now);
    } else if (currentScreen_ == App::ScreenId::Messages) {
        MessagesScreen::update(messageState);
    } else if (currentScreen_ == App::ScreenId::GpsStatus) {
        GpsScreen::update(gpsState);
    } else if (currentScreen_ == App::ScreenId::Astronomy) {
        AstronomyScreen::update(astronomyState, timeState);
    } else if (currentScreen_ == App::ScreenId::Map) {
        MapScreen::update(mapState, stationState, trailState, reference);
    } else if (currentScreen_ == App::ScreenId::Stations) {
        StationsScreen::update(stationState, reference);
    } else if (currentScreen_ == App::ScreenId::StationDetail && selectedStationValid_) {
        StationDetailScreen::update(selectedStation_, reference, now);
    } else if (currentScreen_ == App::ScreenId::StationNavigation && selectedStationValid_) {
        StationNavigationScreen::update(selectedStation_, reference, now);
    } else if (currentScreen_ == App::ScreenId::Weather) {
        WeatherScreen::update(weatherState, reference);
    } else if (currentScreen_ == App::ScreenId::WeatherDetail && selectedWeatherValid_) {
        WeatherDetailScreen::update(selectedWeather_, reference, now);
    } else if (currentScreen_ == App::ScreenId::Tracker) {
        TrackerScreen::update(gpsState, trackerState, settingsState);
    } else if (currentScreen_ == App::ScreenId::Trail) {
        TrailScreen::update(trailState);
    } else if (currentScreen_ == App::ScreenId::Power) {
        PowerScreen::update(powerState);
    } else if (currentScreen_ == App::ScreenId::DigiIgate) {
        DigiIgateScreen::update(digiIgateState, settingsState);
    }
}

App::ScreenId ScreenManager::currentScreen() const {
    return currentScreen_;
}

void ScreenManager::setMessage(const char* text) {
    if (currentScreen_ == App::ScreenId::MainMenu) {
        MenuScreen::setMessage(text);
    } else if (currentScreen_ == App::ScreenId::LoRaStatus) {
        LoRaScreen::setMessage(text);
    } else if (currentScreen_ == App::ScreenId::Messages) {
        MessagesScreen::setMessage(text);
    } else if (currentScreen_ == App::ScreenId::Settings) {
        SettingsScreen::setMessage(text);
    } else if (currentScreen_ == App::ScreenId::Tracker) {
        TrackerScreen::setMessage(text);
    } else if (currentScreen_ == App::ScreenId::Trail) {
        TrailScreen::setMessage(text);
    } else if (currentScreen_ == App::ScreenId::DigiIgate) {
        DigiIgateScreen::setMessage(text);
    } else if (currentScreen_ != App::ScreenId::MainMenu &&
               currentScreen_ != App::ScreenId::Splash) {
        PlaceholderScreen::setMessage(text);
    }
}

void ScreenManager::navigationThunk(App::NavigationAction action, void* context) {
    if (context != nullptr) {
        static_cast<ScreenManager*>(context)->queueNavigation(action);
    }
}

void ScreenManager::queueNavigation(App::NavigationAction action) {
    const std::uint32_t now = millis();
    if (static_cast<std::int32_t>(now - navigationLockedUntil_) < 0) {
        return;
    }

    pendingNavigation_ = action;
    navigationPending_ = true;
}

void ScreenManager::processPendingNavigation() {
    if (!navigationPending_) {
        return;
    }

    const App::NavigationAction action = pendingNavigation_;
    navigationPending_ = false;
    handleNavigation(action);
}

void ScreenManager::handleNavigation(App::NavigationAction action) {
    if (currentScreen_ == App::ScreenId::Splash) {
        return;
    }

    if (currentScreen_ == App::ScreenId::MainMenu) {
        if (action == App::NavigationAction::Up) {
            MenuScreen::moveSelection(-1);
        } else if (action == App::NavigationAction::Down) {
            MenuScreen::moveSelection(1);
        } else if (action == App::NavigationAction::Confirm) {
            selectedMenuItem_ = MenuScreen::selectedIndex();
            showTarget(App::MenuModel::item(selectedMenuItem_).target);
        }
        return;
    }

    if (action == App::NavigationAction::Back) {
        if (currentScreen_ == App::ScreenId::StationNavigation) {
            show(App::ScreenId::StationDetail);
        } else if (currentScreen_ == App::ScreenId::StationDetail) {
            show(App::ScreenId::Stations);
        } else if (currentScreen_ == App::ScreenId::WeatherDetail) {
            show(App::ScreenId::Weather);
        } else {
            showMainMenu();
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::LoRaStatus &&
        action == App::NavigationAction::Confirm &&
        commandHandler_ != nullptr) {
        commandHandler_(App::Command::SendTestPacket, commandContext_);
        return;
    }

    if (currentScreen_ == App::ScreenId::Map) {
        if (commandHandler_ != nullptr) {
            if (action == App::NavigationAction::Up) {
                commandHandler_(App::Command::MapZoomIn, commandContext_);
            } else if (action == App::NavigationAction::Down) {
                commandHandler_(App::Command::MapZoomOut, commandContext_);
            } else if (action == App::NavigationAction::Confirm) {
                commandHandler_(App::Command::MapRecenter, commandContext_);
            }
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::Messages) {
        if (action == App::NavigationAction::Confirm) {
            MessagesScreen::compose();
        } else if (action == App::NavigationAction::Up || action == App::NavigationAction::Down) {
            MessagesScreen::scroll(action == App::NavigationAction::Up ? -1 : 1);
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::Stations) {
        if (action == App::NavigationAction::Up || action == App::NavigationAction::Down) {
            StationsScreen::moveSelection(action == App::NavigationAction::Up ? -1 : 1);
        } else if (action == App::NavigationAction::Confirm &&
                   stationState_ != nullptr && stationState_->count > 0) {
            const std::size_t index = StationsScreen::selectedIndex();
            if (index < stationState_->count) {
                selectedStation_ = stationState_->stations[index];
                selectedStationValid_ = true;
                show(App::ScreenId::StationDetail);
            }
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::StationDetail) {
        if (action == App::NavigationAction::Up || action == App::NavigationAction::Down) {
            StationDetailScreen::scroll(action == App::NavigationAction::Up ? -1 : 1);
        } else if (action == App::NavigationAction::Confirm &&
                   selectedStationValid_ && selectedStation_.hasPosition) {
            show(App::ScreenId::StationNavigation);
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::StationNavigation) {
        if (action == App::NavigationAction::Confirm) {
            show(App::ScreenId::StationDetail);
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::Weather) {
        if (action == App::NavigationAction::Up || action == App::NavigationAction::Down) {
            WeatherScreen::moveSelection(action == App::NavigationAction::Up ? -1 : 1);
        } else if (action == App::NavigationAction::Confirm && weatherState_ != nullptr && weatherState_->count > 0) {
            const std::size_t index = WeatherScreen::selectedIndex();
            if (index < weatherState_->count) {
                selectedWeather_ = weatherState_->stations[index];
                selectedWeatherValid_ = true;
                show(App::ScreenId::WeatherDetail);
            }
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::Tracker) {
        if (action == App::NavigationAction::Confirm) {
            TrackerScreen::save();
        } else if (action == App::NavigationAction::Up || action == App::NavigationAction::Down) {
            TrackerScreen::scroll(action == App::NavigationAction::Up ? -1 : 1);
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::Trail) {
        if (action == App::NavigationAction::Confirm) {
            TrailScreen::togglePause();
        } else if (action == App::NavigationAction::Up || action == App::NavigationAction::Down) {
            TrailScreen::scroll(action == App::NavigationAction::Up ? -1 : 1);
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::DigiIgate) {
        if (action == App::NavigationAction::Confirm) {
            DigiIgateScreen::save();
        } else if (action == App::NavigationAction::Up ||
                   action == App::NavigationAction::Down) {
            DigiIgateScreen::scroll(action == App::NavigationAction::Up ? -1 : 1);
        }
        return;
    }

    if (currentScreen_ == App::ScreenId::Settings &&
        action == App::NavigationAction::Confirm) {
        SettingsScreen::save();
        return;
    }

    if (action == App::NavigationAction::Up || action == App::NavigationAction::Down) {
        const int direction = action == App::NavigationAction::Up ? -1 : 1;
        const std::size_t count = App::MenuModel::count();
        if (direction < 0) {
            selectedMenuItem_ = selectedMenuItem_ == 0 ? count - 1 : selectedMenuItem_ - 1;
        } else {
            selectedMenuItem_ = (selectedMenuItem_ + 1) % count;
        }
        showTarget(App::MenuModel::item(selectedMenuItem_).target);
    }
}

void ScreenManager::show(App::ScreenId screen) {
    currentScreen_ = screen;
    navigationPending_ = false;
    navigationLockedUntil_ = millis() + 180U;
    if (screen == App::ScreenId::Messages) {
        unreadMessageCount_ = 0;
    } else if (screen == App::ScreenId::Stations) {
        newStationCount_ = 0;
    }

    if (screen == App::ScreenId::MainMenu) {
        MenuScreen::create(
            selectedMenuItem_,
            referenceState_,
            makeMenuIndicators(
                gpsState_,
                trackerState_,
                trailState_,
                digiIgateState_,
                unreadMessageCount_,
                newStationCount_));
    } else if (screen == App::ScreenId::LoRaStatus) {
        LoRaScreen::create();
    } else if (screen == App::ScreenId::Diagnostics) {
        DiagnosticsScreen::create();
        if (radioState_ != nullptr) {
            DiagnosticsScreen::update(*radioState_, millis());
        }
    } else if (screen == App::ScreenId::Messages) {
        MessagesScreen::create(messageSendHandler_, messageSendContext_);
    } else if (screen == App::ScreenId::GpsStatus) {
        GpsScreen::create();
        GpsScreen::update(gpsState_);
    } else if (screen == App::ScreenId::Astronomy) {
        AstronomyScreen::create();
        AstronomyScreen::update(astronomyState_, timeState_);
    } else if (screen == App::ScreenId::Map) {
        MapScreen::create();
        if (mapState_ != nullptr && stationState_ != nullptr) {
            MapScreen::update(*mapState_, *stationState_, trailState_, referenceState_);
        }
    } else if (screen == App::ScreenId::Stations) {
        StationsScreen::create();
    } else if (screen == App::ScreenId::StationDetail) {
        StationDetailScreen::create();
        if (selectedStationValid_) {
            StationDetailScreen::update(selectedStation_, referenceState_, millis());
        }
    } else if (screen == App::ScreenId::StationNavigation) {
        StationNavigationScreen::create();
        if (selectedStationValid_) {
            StationNavigationScreen::update(selectedStation_, referenceState_, millis());
        }
    } else if (screen == App::ScreenId::Weather) {
        WeatherScreen::create();
    } else if (screen == App::ScreenId::WeatherDetail) {
        WeatherDetailScreen::create();
        if (selectedWeatherValid_) WeatherDetailScreen::update(selectedWeather_, referenceState_, millis());
    } else if (screen == App::ScreenId::Tracker) {
        TrackerScreen::create(
            settingsState_,
            gpsState_,
            trackerState_,
            trackerSaveHandler_,
            trackerSaveContext_);
    } else if (screen == App::ScreenId::Trail) {
        TrailScreen::create(trailState_, commandHandler_, commandContext_);
    } else if (screen == App::ScreenId::Power) {
        PowerScreen::create();
        PowerScreen::update(powerState_);
    } else if (screen == App::ScreenId::DigiIgate) {
        DigiIgateScreen::create(
            settingsState_,
            digiIgateState_,
            digiIgateSaveHandler_,
            digiIgateSaveContext_);
    } else if (screen == App::ScreenId::Settings) {
        SettingsScreen::create(
            settingsState_,
            settingsSaveHandler_,
            settingsSaveContext_);
    } else {
        const App::MenuModel::MenuItem& item = App::MenuModel::item(selectedMenuItem_);
        PlaceholderScreen::create(item.title, item.description);
    }
    rebuildNavigationBar();
    updateHeaderPower(powerState_);
    updateHeaderTime(timeState_);
}

void ScreenManager::showMainMenu() {
    show(App::ScreenId::MainMenu);
}

void ScreenManager::showTarget(App::ScreenId target) {
    show(target);
}

void ScreenManager::rebuildNavigationBar() {
    if (currentScreen_ != App::ScreenId::Splash) {
        createNavigationBar(navigationThunk, this);
    }
}

void ScreenManager::refreshSelectedStation() {
    if (!selectedStationValid_) {
        return;
    }
    if (stationState_ == nullptr) {
        return;
    }
    for (std::size_t index = 0; index < stationState_->count; ++index) {
        const Services::StationStore::Station& candidate = stationState_->stations[index];
        const bool sameType = candidate.type == selectedStation_.type;
        const bool sameIdentity = candidate.type == Aprs::EntityType::Station
            ? std::strcmp(candidate.callsign, selectedStation_.callsign) == 0
            : std::strcmp(candidate.entityName, selectedStation_.entityName) == 0;
        if (sameType && sameIdentity) {
            selectedStation_ = candidate;
            return;
        }
    }
}

}  // namespace Ui
