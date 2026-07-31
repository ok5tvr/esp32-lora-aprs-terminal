#include <cassert>
#include <cmath>
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
    assert(first.followReference);

    // Dragging the rendered map 64 px to the right moves the geographic
    // center 64 world pixels west. The original GPS point therefore appears
    // approximately 64 px right of the viewport center.
    map.panByPixels(64, 0);
    map.update(2500, true, reference);
    const auto& panned = map.viewState();
    assert(!panned.followReference);
    const auto shiftedReference = Services::MapService::project(
        panned, reference.latitude, reference.longitude);
    assert(shiftedReference.valid);
    assert(shiftedReference.x > 303.0 && shiftedReference.x < 305.0);

    // A new GPS revision must not override a manually panned center.
    Services::PositionReference movedReference = reference;
    movedReference.longitude += 0.25;
    movedReference.revision = 2;
    const double manualCenterLongitude = panned.centerLongitude;
    map.update(2600, true, movedReference);
    assert(!map.viewState().followReference);
    assert(std::fabs(map.viewState().centerLongitude - manualCenterLongitude) < 1e-9);

    map.recenter();
    map.update(2700, true, movedReference);
    assert(map.viewState().followReference);
    const auto recentered = Services::MapService::project(
        map.viewState(), movedReference.latitude, movedReference.longitude);
    assert(recentered.valid);
    assert(recentered.x > 239.0 && recentered.x < 241.0);

    map.zoomIn();
    map.update(3000, true, movedReference);
    assert(map.viewState().zoom == 14U);
    assert(map.viewState().loading);

    map.update(3002, false, movedReference);
    assert(!map.viewState().active);

    std::cout << "map service tests passed\n";
    return 0;
}
