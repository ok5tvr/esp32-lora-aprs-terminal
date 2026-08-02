#pragma once

#include <cstddef>
#include <cstdint>

#include "app_config.h"

namespace Services {

class PowerService {
public:
    enum class ChargerState : std::uint8_t {
        Unknown = 0,
        TriCharge,
        PreCharge,
        ConstantCurrent,
        ConstantVoltage,
        Done,
        Stopped
    };

    enum class HistoryMode : std::uint8_t {
        Unknown = 0,
        Charging,
        Discharging,
        UsbPower,
        Standby
    };

    struct ViewState {
        bool available = false;
        bool batteryConnected = false;
        bool charging = false;
        bool discharging = false;
        bool standby = false;
        bool vbusConnected = false;
        bool vbusGood = false;
        bool criticalBattery = false;
        bool batteryPercentValid = false;
        bool pmicTemperatureValid = false;
        std::uint8_t batteryPercent = 0;
        std::uint16_t batteryVoltageMv = 0;
        std::uint16_t vbusVoltageMv = 0;
        std::uint16_t systemVoltageMv = 0;
        float pmicTemperatureC = 0.0F;
        std::uint16_t configuredChargeCurrentMa = 0;
        std::uint16_t targetChargeVoltageMv = 0;
        ChargerState chargerState = ChargerState::Unknown;

        // Battery percentage history. Mode is kept separately so the UI can
        // colour charging, discharging and USB/standby segments differently.
        std::uint8_t powerHistoryCount = 0;
        std::uint32_t powerHistoryRevision = 0;
        std::uint8_t powerHistoryPercent[AppConfig::POWER_HISTORY_LENGTH] = {};
        HistoryMode powerHistoryMode[AppConfig::POWER_HISTORY_LENGTH] = {};
        std::uint32_t powerHistoryAtMinute[AppConfig::POWER_HISTORY_LENGTH] = {};

        char operatingText[24] = "--";
        char chargerText[28] = "neznamy stav";
        char lastEvent[48] = "AXP2101 nebyl inicializovan";
        std::uint32_t revision = 0;
    };

    bool begin();
    void update(std::uint32_t now);
    const ViewState& viewState() const;

private:
    void readState(bool firstRead);
    void updateLastEvent(const ViewState& previous, bool firstRead);
    HistoryMode currentHistoryMode() const;
    void appendPowerHistory(std::uint32_t now, bool force = false);
    bool loadPowerHistory(std::uint32_t now);
    bool savePowerHistory() const;
    void advanceHistoryClock(std::uint32_t now);

    ViewState view_;
    std::uint32_t lastPollAt_ = 0;
    std::uint32_t lastHistorySampleAt_ = 0;
    std::uint32_t historyClockAt_ = 0;
    std::uint32_t historyClockRemainderMs_ = 0;
    std::uint32_t historyMinuteNow_ = 0;
    HistoryMode lastHistoryMode_ = HistoryMode::Unknown;
    std::uint8_t lastHistoryPercent_ = 0;
    std::uint8_t pendingHistoryPercent_ = 0;
    std::uint8_t pendingHistoryConfirmations_ = 0;
    bool historyStarted_ = false;
    std::uint32_t localizationRevision_ = 0;
};

}  // namespace Services
