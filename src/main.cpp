#include <Arduino.h>

#include "app/app_controller.h"

namespace {
App::AppController application;
}

void setup() {
    application.begin();
}

void loop() {
    application.update();
}
