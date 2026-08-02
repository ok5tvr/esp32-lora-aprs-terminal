#include <cassert>
#include <cstring>
#include <iostream>

#include <Arduino.h>
#include <Preferences.h>
#include <XPowersLib.h>

#include "app_config.h"
#include "services/power_service.h"

int main() {
    powerPreferencesTestReset();
    powerTestMillis = 0U;
    powerTestBatteryPercent = 74;
    powerTestCharging = false;
    powerTestDischarging = true;
    powerTestVbusConnected = false;
    powerTestVbusGood = false;

    Services::PowerService service;
    assert(service.begin());

    const Services::PowerService::ViewState& initial = service.viewState();
    assert(initial.available);
    assert(initial.batteryConnected);
    assert(initial.batteryPercentValid);
    assert(initial.batteryPercent == 74U);
    assert(initial.batteryVoltageMv == 3900U);
    assert(initial.systemVoltageMv == 4020U);
    assert(initial.pmicTemperatureValid);
    assert(initial.configuredChargeCurrentMa == 200U);
    assert(initial.targetChargeVoltageMv == 4100U);
    assert(initial.chargerState == Services::PowerService::ChargerState::Stopped);
    assert(std::strcmp(initial.lastEvent, "Provoz z akumulatoru") == 0);
    assert(initial.powerHistoryCount == 1U);
    assert(initial.powerHistoryPercent[0] == 74U);
    assert(initial.powerHistoryMode[0] == Services::PowerService::HistoryMode::Discharging);
    assert(initial.powerHistoryAtMinute[0] == 0U);
    assert(powerPreferencesTestWriteCount() == 1U);

    // A normal telemetry poll with the same percentage creates no point.
    powerTestMillis = AppConfig::POWER_POLL_INTERVAL_MS;
    service.update(powerTestMillis);
    assert(service.viewState().powerHistoryCount == 1U);
    assert(powerPreferencesTestWriteCount() == 1U);

    // A one-percent decrease is confirmed by two consecutive polls before it
    // is persisted, preventing rapid writes caused by a fluctuating gauge.
    powerTestBatteryPercent = 73;
    powerTestMillis += AppConfig::POWER_POLL_INTERVAL_MS;
    service.update(powerTestMillis);
    assert(service.viewState().powerHistoryCount == 1U);
    assert(powerPreferencesTestWriteCount() == 1U);

    powerTestMillis += AppConfig::POWER_POLL_INTERVAL_MS;
    service.update(powerTestMillis);
    assert(service.viewState().powerHistoryCount == 2U);
    assert(service.viewState().powerHistoryPercent[1] == 73U);
    assert(service.viewState().powerHistoryMode[1] == Services::PowerService::HistoryMode::Discharging);
    assert(powerPreferencesTestWriteCount() == 2U);

    // An unchanged value is not duplicated.
    powerTestMillis += AppConfig::POWER_POLL_INTERVAL_MS;
    service.update(powerTestMillis);
    assert(service.viewState().powerHistoryCount == 2U);
    assert(powerPreferencesTestWriteCount() == 2U);

    // A power-mode transition is recorded even when percentage is unchanged.
    powerTestCharging = true;
    powerTestDischarging = false;
    powerTestVbusConnected = true;
    powerTestVbusGood = true;
    powerTestMillis += AppConfig::POWER_POLL_INTERVAL_MS;
    service.update(powerTestMillis);
    assert(service.viewState().powerHistoryCount == 3U);
    assert(service.viewState().powerHistoryPercent[2] == 73U);
    assert(service.viewState().powerHistoryMode[2] == Services::PowerService::HistoryMode::Charging);
    assert(powerPreferencesTestWriteCount() == 3U);

    // A stable value is checkpointed after the maximum one-hour interval.
    powerTestMillis += AppConfig::POWER_HISTORY_MAX_INTERVAL_MS;
    service.update(powerTestMillis);
    assert(service.viewState().powerHistoryCount == 4U);
    assert(service.viewState().powerHistoryPercent[3] == 73U);
    assert(service.viewState().powerHistoryAtMinute[3] >= 60U);
    assert(powerPreferencesTestWriteCount() == 4U);

    // Simulate an ESP32 restart. The NVS-backed history must be restored and
    // begin() must not append a duplicate when percentage and mode are equal.
    powerTestMillis = 0U;
    Services::PowerService restartedService;
    assert(restartedService.begin());
    const Services::PowerService::ViewState& restored = restartedService.viewState();
    assert(restored.powerHistoryCount == 4U);
    assert(restored.powerHistoryPercent[0] == 74U);
    assert(restored.powerHistoryPercent[3] == 73U);
    assert(restored.powerHistoryMode[3] == Services::PowerService::HistoryMode::Charging);
    assert(restored.powerHistoryAtMinute[3] >= restored.powerHistoryAtMinute[0]);
    assert(powerPreferencesTestWriteCount() == 4U);

    // A confirmed one-percent increase after restart appends after the restored
    // timeline.
    powerTestBatteryPercent = 74;
    powerTestMillis = AppConfig::POWER_POLL_INTERVAL_MS;
    restartedService.update(powerTestMillis);
    assert(restartedService.viewState().powerHistoryCount == 4U);
    powerTestMillis += AppConfig::POWER_POLL_INTERVAL_MS;
    restartedService.update(powerTestMillis);
    const Services::PowerService::ViewState& afterRestart = restartedService.viewState();
    assert(afterRestart.powerHistoryCount == 5U);
    assert(afterRestart.powerHistoryPercent[4] == 74U);
    assert(afterRestart.powerHistoryAtMinute[4] > afterRestart.powerHistoryAtMinute[3]);
    assert(powerPreferencesTestWriteCount() == 5U);

    // Overflow the ring buffer. Only the newest 96 points must remain and the
    // complete ring must still restore from one NVS blob.
    for (std::size_t index = 0; index < AppConfig::POWER_HISTORY_LENGTH + 8U; ++index) {
        powerTestBatteryPercent = (index % 2U == 0U) ? 73 : 74;
        for (std::uint8_t confirmation = 0;
             confirmation < AppConfig::POWER_HISTORY_PERCENT_CONFIRMATIONS;
             ++confirmation) {
            powerTestMillis += AppConfig::POWER_POLL_INTERVAL_MS;
            restartedService.update(powerTestMillis);
        }
    }
    assert(restartedService.viewState().powerHistoryCount == AppConfig::POWER_HISTORY_LENGTH);

    powerTestMillis = 0U;
    Services::PowerService fullHistoryRestart;
    assert(fullHistoryRestart.begin());
    assert(fullHistoryRestart.viewState().powerHistoryCount == AppConfig::POWER_HISTORY_LENGTH);
    assert(fullHistoryRestart.viewState().powerHistoryPercent[AppConfig::POWER_HISTORY_LENGTH - 1U] ==
           restartedService.viewState().powerHistoryPercent[AppConfig::POWER_HISTORY_LENGTH - 1U]);

    std::cout << "power service tests passed\n";
    return 0;
}
