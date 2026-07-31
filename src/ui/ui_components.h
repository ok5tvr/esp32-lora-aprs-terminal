#pragma once

#include <lvgl.h>

#include "app/app_types.h"
#include "services/power_service.h"
#include "services/time_service.h"

namespace Ui {

using NavigationHandler = void (*)(App::NavigationAction action, void* context);

void resetScreen();
void createHeader(const char* title);
void createClockHeader();
void updateHeaderPower(const Services::PowerService::ViewState& state);
void updateHeaderTime(const Services::TimeService::ViewState& state);
void createNavigationBar(NavigationHandler handler, void* context);

}  // namespace Ui
