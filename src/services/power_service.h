#pragma once

#include <cstdint>

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

    ViewState view_;
    std::uint32_t lastPollAt_ = 0;
    std::uint32_t localizationRevision_ = 0;
};

}  // namespace Services
