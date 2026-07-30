#include <cassert>
#include <cstring>
#include <iostream>

#include "drivers/sd_card_driver.h"
#include "services/map_service.h"

namespace Drivers::SdCard {
Status testStatus{true, 1, 0, 0, 0};
bool begin() { return true; }
const Status& status() { return testStatus; }
void refreshUsage() {}
}  // namespace Drivers::SdCard

int main() {
    Services::MapService map;
    assert(map.begin());
    assert(map.viewState().initialized);
    assert(map.viewState().pixels != nullptr);

    Services::PositionReference reference;
    reference.valid = true;
    reference.fromGps = true;
    reference.latitude = 49.786333;
    reference.longitude = 13.285000;
    reference.revision = 1;

    for (std::uint32_t now = 0; now < 2000; now += 2) {
        map.update(now, true, reference);
        if (!map.viewState().loading && map.viewState().tileJobsCompleted > 0U) {
            break;
        }
    }

    const auto& first = map.viewState();
    assert(first.centerValid);
    assert(first.zoom == 13U);
    assert(first.tileJobsTotal > 0U);
    assert(first.tileJobsCompleted == first.tileJobsTotal);
    assert(!first.loading);
    assert(first.missingTiles == 0U);
    assert(std::strstr(first.statusText, "nactena") != nullptr);

    const auto center = Services::MapService::project(
        first, reference.latitude, reference.longitude);
    assert(center.valid);
    assert(center.x > 239.0 && center.x < 241.0);
    assert(center.y > 100.0 && center.y < 102.0);

    map.zoomIn();
    map.update(3000, true, reference);
    assert(map.viewState().zoom == 14U);
    assert(map.viewState().loading);

    map.update(3002, false, reference);
    assert(!map.viewState().active);

    std::cout << "map service tests passed\n";
    return 0;
}
