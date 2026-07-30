#pragma once

#include "app/app_types.h"
#include "services/trail_service.h"

namespace Ui {
namespace TrailScreen {

void create(
    const Services::TrailService::ViewState& state,
    App::CommandHandler commandHandler,
    void* commandContext);
void update(const Services::TrailService::ViewState& state);
void togglePause();
void scroll(int direction);
void setMessage(const char* text);

}  // namespace TrailScreen
}  // namespace Ui
