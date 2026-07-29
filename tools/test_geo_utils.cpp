#include <cassert>
#include <cstring>
#include <iostream>

#include "services/geo_utils.h"

int main() {
    char locator[7] = {};
    assert(Services::maidenheadLocator(49.786333, 13.285000, locator, sizeof(locator)));
    assert(std::strcmp(locator, "JN69PS") == 0);

    assert(Services::maidenheadLocator(0.0, 0.0, locator, sizeof(locator)));
    assert(std::strcmp(locator, "JJ00AA") == 0);

    assert(!Services::maidenheadLocator(91.0, 13.0, locator, sizeof(locator)));

    const Services::DistanceBearing db = Services::calculateDistanceBearing(
        49.786333, 13.285000, 50.0755, 14.4378);
    assert(db.valid);
    assert(db.distanceKm > 85.0 && db.distanceKm < 95.0);
    assert(db.bearingDegrees > 60.0 && db.bearingDegrees < 80.0);

    std::cout << "geo_utils tests passed\n";
    return 0;
}
