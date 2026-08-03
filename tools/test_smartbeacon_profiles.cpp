#include <cassert>
#include <cmath>

#include "app/smartbeacon_profiles.h"

int main() {
    const auto& car = App::smartBeaconProfileDefinition(App::SmartBeaconProfile::Car);
    const auto& bicycle = App::smartBeaconProfileDefinition(App::SmartBeaconProfile::Bicycle);
    const auto& walking = App::smartBeaconProfileDefinition(App::SmartBeaconProfile::Walking);

    assert(App::smartBeaconIntervalSeconds(car, 0.0F) == 1800U);
    assert(App::smartBeaconIntervalSeconds(car, 70.0F) == 120U);
    assert(App::smartBeaconIntervalSeconds(car, 35.0F) == 240U);

    assert(App::smartBeaconIntervalSeconds(bicycle, 0.0F) == 1200U);
    assert(App::smartBeaconIntervalSeconds(bicycle, 30.0F) == 90U);
    assert(App::smartBeaconIntervalSeconds(bicycle, 15.0F) == 180U);

    assert(App::smartBeaconIntervalSeconds(walking, 0.0F) == 900U);
    assert(App::smartBeaconIntervalSeconds(walking, 7.0F) == 120U);
    assert(App::smartBeaconIntervalSeconds(walking, 3.5F) == 240U);

    assert(std::fabs(App::smartBeaconTurnThresholdDegrees(car, 50.0F) - 34.8F) < 0.01F);
    assert(App::smartBeaconTurnThresholdDegrees(walking, 3.0F) >
           App::smartBeaconTurnThresholdDegrees(bicycle, 15.0F));
    return 0;
}
