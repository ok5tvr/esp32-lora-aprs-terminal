#pragma once

#include <cstddef>
#include <cstdint>

#include "app/app_types.h"
#include "services/geo_utils.h"
#include "services/gps_service.h"
#include "services/map_service.h"
#include "services/power_service.h"
#include "services/radio_service.h"
#include "services/settings_service.h"
#include "services/trail_service.h"
#include "services/tracker_service.h"

namespace Ui {

class ScreenManager {
public:
    void begin(
        App::CommandHandler commandHandler,
        void* commandContext,
        App::MessageSendHandler messageSendHandler,
        void* messageSendContext,
        App::SettingsSaveHandler settingsSaveHandler,
        void* settingsSaveContext,
        App::DigiIgateSettingsSaveHandler digiIgateSaveHandler,
        void* digiIgateSaveContext,
        App::TrackerSettingsSaveHandler trackerSaveHandler,
        void* trackerSaveContext);
    void update(
        std::uint32_t now,
        const Services::RadioService::ViewState& radioState,
        const Services::MessageStore::ViewState& messageState,
        const Services::StationStore::ViewState& stationState,
        const Services::WeatherStore::ViewState& weatherState,
        const Services::GpsService::ViewState& gpsState,
        const Services::TrackerService::ViewState& trackerState,
        const Services::TrailService::ViewState& trailState,
        const Services::PowerService::ViewState& powerState,
        const Services::DigiIgateService::ViewState& digiIgateState,
        const Services::PositionReference& reference,
        const Services::MapService::ViewState& mapState,
        const Services::SettingsService::ViewState& settingsState);
    void setMessage(const char* text);
    App::ScreenId currentScreen() const;

private:
    static void navigationThunk(App::NavigationAction action, void* context);
    void queueNavigation(App::NavigationAction action);
    void processPendingNavigation();
    void handleNavigation(App::NavigationAction action);
    void show(App::ScreenId screen);
    void showMainMenu();
    void showTarget(App::ScreenId target);
    void rebuildNavigationBar();
    void refreshSelectedStation();

    App::CommandHandler commandHandler_ = nullptr;
    void* commandContext_ = nullptr;
    App::MessageSendHandler messageSendHandler_ = nullptr;
    void* messageSendContext_ = nullptr;
    App::SettingsSaveHandler settingsSaveHandler_ = nullptr;
    void* settingsSaveContext_ = nullptr;
    App::DigiIgateSettingsSaveHandler digiIgateSaveHandler_ = nullptr;
    void* digiIgateSaveContext_ = nullptr;
    App::TrackerSettingsSaveHandler trackerSaveHandler_ = nullptr;
    void* trackerSaveContext_ = nullptr;
    Services::SettingsService::ViewState settingsState_;
    const Services::RadioService::ViewState* radioState_ = nullptr;
    Services::GpsService::ViewState gpsState_;
    const Services::StationStore::ViewState* stationState_ = nullptr;
    const Services::WeatherStore::ViewState* weatherState_ = nullptr;
    Services::StationStore::Station selectedStation_;
    bool selectedStationValid_ = false;
    Services::WeatherStore::WeatherStation selectedWeather_;
    bool selectedWeatherValid_ = false;
    Services::TrackerService::ViewState trackerState_;
    Services::TrailService::ViewState trailState_;
    Services::PowerService::ViewState powerState_;
    Services::DigiIgateService::ViewState digiIgateState_;
    Services::PositionReference referenceState_;
    const Services::MapService::ViewState* mapState_ = nullptr;
    App::ScreenId currentScreen_ = App::ScreenId::Splash;
    std::size_t selectedMenuItem_ = 0;
    std::uint32_t lastRefreshAt_ = 0;
    App::NavigationAction pendingNavigation_ = App::NavigationAction::Back;
    bool navigationPending_ = false;
    std::uint32_t navigationLockedUntil_ = 0;
    std::uint32_t observedMessageEvents_ = 0;
    std::uint32_t observedStationEvents_ = 0;
    std::uint8_t unreadMessageCount_ = 0;
    std::uint8_t newStationCount_ = 0;
};

}  // namespace Ui
