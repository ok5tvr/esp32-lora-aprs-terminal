#include <cassert>

#define private public
#include "services/tracker_service.h"
#undef private

int main() {
    Services::TrackerService tracker;
    const auto& car = App::smartBeaconProfileDefinition(App::SmartBeaconProfile::Car);

    assert(tracker.motionBeaconDue(1000U, 7.0F, true, car) ==
           Services::TrackerService::BeaconReason::None);
    assert(tracker.motionBeaconDue(8999U, 7.0F, true, car) ==
           Services::TrackerService::BeaconReason::None);
    assert(tracker.motionBeaconDue(9000U, 7.0F, true, car) ==
           Services::TrackerService::BeaconReason::StartMoving);

    tracker.moving_ = true;
    tracker.startCandidateAtMs_ = 0;
    assert(tracker.motionBeaconDue(10000U, 2.0F, true, car) ==
           Services::TrackerService::BeaconReason::None);
    assert(tracker.motionBeaconDue(54999U, 2.0F, true, car) ==
           Services::TrackerService::BeaconReason::None);
    assert(tracker.motionBeaconDue(55000U, 2.0F, true, car) ==
           Services::TrackerService::BeaconReason::Stopped);

    tracker.hasTransmitted_ = true;
    tracker.moving_ = true;
    tracker.lastCourseValid_ = true;
    tracker.lastCourseAtTransmit_ = 359.0F;
    tracker.view_.lastTransmitAtMs = 0;
    assert(!tracker.cornerBeaconDue(16000U, 50.0F, 2.0F, true, car));
    assert(tracker.cornerBeaconDue(16000U, 50.0F, 40.0F, true, car));
    Services::SettingsService::ViewState settings;
    settings.trackerMode = App::TrackerBeaconMode::SmartBeacon;
    settings.trackerSmartProfile = App::SmartBeaconProfile::Car;
    tracker.moving_ = false;
    tracker.pendingTx_ = true;
    tracker.pendingReason_ = Services::TrackerService::BeaconReason::SpeedInterval;
    tracker.pendingSpeedValid_ = true;
    tracker.pendingSpeedKmh_ = 10.0F;
    tracker.confirmTransmission(20000U, settings);
    assert(tracker.moving_);
    return 0;
}
