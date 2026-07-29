#pragma once

#include "app/app_types.h"
#include "services/digi_igate_service.h"
#include "services/settings_service.h"

namespace Ui {
namespace DigiIgateScreen {

void create(
    const Services::SettingsService::ViewState& settings,
    const Services::DigiIgateService::ViewState& state,
    App::DigiIgateSettingsSaveHandler saveHandler,
    void* saveContext);
void update(
    const Services::DigiIgateService::ViewState& state,
    const Services::SettingsService::ViewState& settings);
void processPending();
void save();
void scroll(int direction);
void setMessage(const char* text);

}  // namespace DigiIgateScreen
}  // namespace Ui
