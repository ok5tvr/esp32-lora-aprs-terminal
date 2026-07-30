#include <cassert>
#include <cstring>
#include <iostream>

#include "services/power_service.h"

int main() {
    Services::PowerService service;
    assert(service.begin());

    const Services::PowerService::ViewState& state = service.viewState();
    assert(state.available);
    assert(state.batteryConnected);
    assert(state.batteryPercentValid);
    assert(state.batteryPercent == 74U);
    assert(state.batteryVoltageMv == 3900U);
    assert(state.systemVoltageMv == 4020U);
    assert(state.pmicTemperatureValid);
    assert(state.configuredChargeCurrentMa == 200U);
    assert(state.targetChargeVoltageMv == 4100U);
    assert(state.chargerState == Services::PowerService::ChargerState::Stopped);
    assert(std::strcmp(state.lastEvent, "Provoz z akumulatoru") == 0);

    std::cout << "power service tests passed\n";
    return 0;
}
