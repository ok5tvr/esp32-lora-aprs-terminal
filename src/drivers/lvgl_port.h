#pragma once

namespace Drivers {
namespace LvglPort {

bool begin();
void update();
void setTouchWakeOnly(bool enabled);
bool consumeTouchActivity();

}  // namespace LvglPort
}  // namespace Drivers
