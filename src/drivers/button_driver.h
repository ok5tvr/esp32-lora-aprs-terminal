#pragma once

#include <cstdint>

namespace Drivers {

class ButtonDriver {
public:
    bool begin();
    void update(std::uint32_t now);
    bool consumeBootClick();

private:
    bool initialized_ = false;
    bool rawPressed_ = false;
    bool stablePressed_ = false;
    bool ignoreUntilReleased_ = false;
    bool bootClickPending_ = false;
    std::uint32_t rawChangedAtMs_ = 0;
    std::uint32_t pressedAtMs_ = 0;
    std::uint32_t lastClickAtMs_ = 0;
};

}  // namespace Drivers
