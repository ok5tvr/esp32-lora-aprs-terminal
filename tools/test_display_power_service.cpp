#include <cassert>
#include <cstdint>

#include "drivers/display_driver.h"
#include "drivers/lvgl_port.h"
#include "services/display_power_service.h"

namespace {
std::uint8_t brightness = 255;
bool wakeOnly = false;
}

namespace Drivers {
namespace Display {
bool begin() { return true; }
bool setBacklightPercent(std::uint8_t percent) { brightness = percent; return true; }
std::uint8_t backlightPercent() { return brightness; }
void drawRgb565Bitmap(std::int16_t, std::int16_t, std::uint16_t*, std::uint16_t, std::uint16_t, bool) {}
}
namespace LvglPort {
bool begin() { return true; }
void update() {}
void setTouchWakeOnly(bool enabled) { wakeOnly = enabled; }
bool consumeTouchActivity() { return false; }
}
}

int main() {
    Services::SettingsService::ViewState settings;
    settings.batteryBrightnessPercent = 70;
    settings.displayTimeoutSeconds = 60;

    Services::PowerService::ViewState power;
    power.available = true;
    power.batteryConnected = true;
    power.vbusConnected = false;
    power.vbusGood = false;

    Services::DisplayPowerService service;
    assert(service.begin(1000, settings, power));
    assert(brightness == 70);
    assert(service.viewState().stage == Services::DisplayPowerService::Stage::Full);

    service.update(30999, settings, power, false);
    assert(brightness == 70);
    service.update(31000, settings, power, false);
    assert(brightness == 15);
    assert(service.viewState().displayDimmed);

    service.update(60999, settings, power, false);
    assert(brightness == 15);
    service.update(61000, settings, power, false);
    assert(brightness == 0);
    assert(service.viewState().displayOff);
    assert(wakeOnly);

    assert(service.update(62000, settings, power, true));
    assert(brightness == 70);
    assert(!service.viewState().displayOff);
    assert(!wakeOnly);

    service.update(92000, settings, power, false);
    assert(brightness == 15);
    assert(!service.update(93000, settings, power, true));
    assert(brightness == 70);
    assert(service.viewState().stage == Services::DisplayPowerService::Stage::Full);

    power.vbusConnected = true;
    power.vbusGood = true;
    service.update(94000, settings, power, false);
    assert(brightness == 100);
    service.update(999999, settings, power, false);
    assert(brightness == 100);
    assert(!service.viewState().displayOff);
    assert(!service.viewState().displayDimmed);

    power.vbusConnected = false;
    power.vbusGood = false;
    settings.displayTimeoutSeconds = 0;
    service.update(1000000, settings, power, false);
    assert(brightness == 70);
    service.update(1030000, settings, power, false);
    assert(brightness == 15);
    assert(!service.viewState().displayOff);

    settings.batteryBrightnessPercent = 10;
    service.update(1030001, settings, power, true);
    assert(brightness == 10);
    service.update(1060001, settings, power, false);
    assert(brightness == 10);  // dim stage never raises a user-selected lower brightness
    return 0;
}
