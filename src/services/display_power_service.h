#pragma once

#include <cstdint>

#include "services/power_service.h"
#include "services/settings_service.h"

namespace Services {

class DisplayPowerService {
public:
    enum class Stage : std::uint8_t {
        Full = 0,
        Dimmed,
        Off
    };

    struct ViewState {
        bool batteryMode = false;
        bool displayOff = false;
        bool displayDimmed = false;
        Stage stage = Stage::Full;
        std::uint8_t appliedBrightnessPercent = 100;
        std::uint32_t revision = 0;
    };

    bool begin(
        std::uint32_t now,
        const SettingsService::ViewState& settings,
        const PowerService::ViewState& power);

    // Returns true when this update woke a previously blanked display because
    // of user activity. The caller can use this to consume a wake-only button.
    bool update(
        std::uint32_t now,
        const SettingsService::ViewState& settings,
        const PowerService::ViewState& power,
        bool userActivity);

    const ViewState& viewState() const;

private:
    static bool isBatteryMode(const PowerService::ViewState& power);
    std::uint8_t fullBrightness(const SettingsService::ViewState& settings) const;
    std::uint8_t dimBrightness(const SettingsService::ViewState& settings) const;
    void applyBrightness(std::uint8_t percent, bool force = false);
    bool wake(
        std::uint32_t now,
        const SettingsService::ViewState& settings);
    void setFull(const SettingsService::ViewState& settings, bool force = false);
    void setDimmed(const SettingsService::ViewState& settings);
    void blank();

    ViewState view_;
    bool initialized_ = false;
    std::uint32_t lastActivityAt_ = 0;
    std::uint8_t lastRequestedBrightnessPercent_ = 0xFFU;
};

}  // namespace Services
