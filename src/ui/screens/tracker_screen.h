#pragma once

#include "app/app_types.h"
#include "services/gps_service.h"
#include "services/settings_service.h"
#include "services/tracker_service.h"

namespace Ui {
namespace TrackerScreen {

void create(
    const Services::SettingsService::ViewState& settings,
    const Services::GpsService::ViewState& gps,
    const Services::TrackerService::ViewState& tracker,
    App::TrackerSettingsSaveHandler saveHandler,
    void* saveContext);
void update(
    const Services::GpsService::ViewState& gps,
    const Services::TrackerService::ViewState& tracker,
    const Services::SettingsService::ViewState& settings);
void save();
void scroll(int direction);
void setMessage(const char* text);

}  // namespace TrackerScreen
}  // namespace Ui
