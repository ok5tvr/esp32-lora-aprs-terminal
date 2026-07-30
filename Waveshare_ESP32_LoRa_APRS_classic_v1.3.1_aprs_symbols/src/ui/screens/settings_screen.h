#pragma once

#include "app/app_types.h"
#include "services/settings_service.h"

namespace Ui {
namespace SettingsScreen {

void create(
    const Services::SettingsService::ViewState& state,
    App::SettingsSaveHandler saveHandler,
    void* saveContext);
void processPending();
void save();
void setMessage(const char* text);

}  // namespace SettingsScreen
}  // namespace Ui
