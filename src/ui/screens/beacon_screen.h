#pragma once

#include "app/app_types.h"
#include "services/gps_service.h"
#include "services/settings_service.h"

namespace Ui {
namespace BeaconScreen {

void create(
    const Services::SettingsService::ViewState& settings,
    const Services::GpsService::ViewState& gps,
    App::BeaconActionHandler actionHandler,
    void* actionContext);
void update(
    const Services::GpsService::ViewState& gps,
    const Services::SettingsService::ViewState& settings);
void save();
void send();
void processPending();
void scroll(int direction);
void setMessage(const char* text);

}  // namespace BeaconScreen
}  // namespace Ui
