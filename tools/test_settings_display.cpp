#include <cassert>
#include <cmath>
#include <cstring>

#include "app_config.h"
#include "lora_profile.h"
#include "services/settings_service.h"

namespace {

bool near(float left, float right) {
    return std::fabs(left - right) < 0.001F;
}

}  // namespace

int main() {
    Services::SettingsService service;
    assert(service.begin());
    assert(service.viewState().batteryBrightnessPercent == 70);
    assert(service.viewState().displayTimeoutSeconds == 60);
    assert(service.viewState().uiLanguage == App::UiLanguage::Czech);
    assert(service.viewState().loraPreset == App::LoRaPreset::CzeAprs);
    assert(near(service.viewState().loraFrequencyMHz, 433.775F));
    assert(near(service.viewState().loraBandwidthKHz, 125.0F));
    assert(service.viewState().loraSpreadingFactor == 12);
    assert(service.viewState().loraCodingRate == 5);
    assert(service.viewState().loraOutputPowerDbm == 10);

    char error[160] = {};
    assert(!service.save(
        "OK5TVR-15", 49.7, 13.3, 5, 60,
        App::UiLanguage::Czech,
        false,
        App::LoRaPreset::CzeAprs,
        LoRaProfile::FREQUENCY_MHZ,
        LoRaProfile::BANDWIDTH_KHZ,
        LoRaProfile::SPREADING_FACTOR,
        LoRaProfile::CODING_RATE,
        LoRaProfile::OUTPUT_POWER_DBM,
        error, sizeof(error)));
    assert(!service.save(
        "OK5TVR-15", 49.7, 13.3, 70, 77,
        App::UiLanguage::Czech,
        false,
        App::LoRaPreset::CzeAprs,
        LoRaProfile::FREQUENCY_MHZ,
        LoRaProfile::BANDWIDTH_KHZ,
        LoRaProfile::SPREADING_FACTOR,
        LoRaProfile::CODING_RATE,
        LoRaProfile::OUTPUT_POWER_DBM,
        error, sizeof(error)));
    assert(!service.save(
        "OK5TVR-15", 49.7, 13.3, 70, 60,
        App::UiLanguage::Czech,
        false,
        App::LoRaPreset::Custom,
        600.0F, 125.0F, 12, 5, 10,
        error, sizeof(error)));

    assert(service.save(
        "OK5TVR-15", 49.7, 13.3, 45, 120,
        App::UiLanguage::English,
        false,
        App::LoRaPreset::Custom,
        433.900F, 250.0F, 9, 6, 14,
        error, sizeof(error)));
    assert(service.viewState().batteryBrightnessPercent == 45);
    assert(service.viewState().displayTimeoutSeconds == 120);
    assert(service.viewState().uiLanguage == App::UiLanguage::English);
    assert(service.viewState().loraPreset == App::LoRaPreset::Custom);
    assert(near(service.viewState().loraFrequencyMHz, 433.900F));
    assert(near(service.viewState().loraBandwidthKHz, 250.0F));
    assert(service.viewState().loraSpreadingFactor == 9);
    assert(service.viewState().loraCodingRate == 6);
    assert(service.viewState().loraOutputPowerDbm == 14);

    assert(service.saveTracker(
        true,
        false,
        App::TrackerPositionSource::Gps,
        App::TrackerPositionFormat::Compressed,
        App::TrackerBeaconMode::FixedInterval,
        App::SmartBeaconProfile::Bicycle,
        App::TrackerSymbol::Car,
        App::AprsPath::Wide2_2,
        "Mobile tracker",
        300,
        error,
        sizeof(error)));
    assert(service.saveBeacon(
        App::TrackerPositionSource::DefaultPosition,
        App::AprsPath::Wide1_1,
        "QTH beacon",
        error,
        sizeof(error)));

    Services::SettingsService reloaded;
    assert(reloaded.begin());
    assert(reloaded.viewState().batteryBrightnessPercent == 45);
    assert(reloaded.viewState().displayTimeoutSeconds == 120);
    assert(reloaded.viewState().uiLanguage == App::UiLanguage::English);
    assert(reloaded.viewState().loraPreset == App::LoRaPreset::Custom);
    assert(near(reloaded.viewState().loraFrequencyMHz, 433.900F));
    assert(near(reloaded.viewState().loraBandwidthKHz, 250.0F));
    assert(reloaded.viewState().loraSpreadingFactor == 9);
    assert(reloaded.viewState().loraCodingRate == 6);
    assert(reloaded.viewState().loraOutputPowerDbm == 14);
    assert(reloaded.viewState().trackerSmartProfile == App::SmartBeaconProfile::Bicycle);
    assert(reloaded.viewState().trackerPath == App::AprsPath::Wide2_2);
    assert(std::strcmp(reloaded.viewState().trackerComment, "Mobile tracker") == 0);
    assert(reloaded.viewState().beaconSource == App::TrackerPositionSource::DefaultPosition);
    assert(reloaded.viewState().beaconPath == App::AprsPath::Wide1_1);
    assert(std::strcmp(reloaded.viewState().beaconComment, "QTH beacon") == 0);

    assert(reloaded.save(
        "OK5TVR-15", 49.7, 13.3, 70, 60,
        App::UiLanguage::Czech,
        false,
        App::LoRaPreset::CzeAprs,
        420.0F, 500.0F, 7, 8, 17,
        error, sizeof(error)));
    assert(reloaded.viewState().uiLanguage == App::UiLanguage::Czech);
    assert(reloaded.viewState().loraPreset == App::LoRaPreset::CzeAprs);
    assert(near(reloaded.viewState().loraFrequencyMHz, LoRaProfile::FREQUENCY_MHZ));
    assert(near(reloaded.viewState().loraBandwidthKHz, LoRaProfile::BANDWIDTH_KHZ));
    assert(reloaded.viewState().loraSpreadingFactor == LoRaProfile::SPREADING_FACTOR);
    assert(reloaded.viewState().loraCodingRate == LoRaProfile::CODING_RATE);
    assert(reloaded.viewState().loraOutputPowerDbm == LoRaProfile::OUTPUT_POWER_DBM);
    return 0;
}
