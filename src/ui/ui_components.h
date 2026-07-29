#pragma once

#include <lvgl.h>

#include "app/app_types.h"

namespace Ui {

using NavigationHandler = void (*)(App::NavigationAction action, void* context);

void resetScreen();
void createHeader(const char* title);
void createNavigationBar(NavigationHandler handler, void* context);

}  // namespace Ui
