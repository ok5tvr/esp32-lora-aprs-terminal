#include "services/power_service.h"

#include <Arduino.h>
#include <Wire.h>

#include <cmath>
#include <cstddef>
#include <cstdio>

#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

#include "app_config.h"
#include "app/localization.h"
#include "app_log.h"
#include "board_pins.h"

namespace Services {
namespace {

XPowersPMU pmu;
bool pmuReady = false;

const char* chargerStateText(PowerService::ChargerState state) {
    switch (state) {
        case PowerService::ChargerState::TriCharge:
            return App::Localization::text("tri-charge", "tri-charge");
        case PowerService::ChargerState::PreCharge:
            return App::Localization::text("prednabijeni", "pre-charge");
        case PowerService::ChargerState::ConstantCurrent:
            return App::Localization::text("konstantni proud", "constant current");
        case PowerService::ChargerState::ConstantVoltage:
            return App::Localization::text("konstantni napeti", "constant voltage");
        case PowerService::ChargerState::Done:
            return App::Localization::text("nabijeni dokonceno", "charging complete");
        case PowerService::ChargerState::Stopped:
            return App::Localization::text("nenabiji", "not charging");
        case PowerService::ChargerState::Unknown:
        default:
            return App::Localization::text("neznamy stav", "unknown state");
    }
}

PowerService::ChargerState decodeChargerState(std::uint8_t state) {
    switch (state) {
        case XPOWERS_AXP2101_CHG_TRI_STATE:
            return PowerService::ChargerState::TriCharge;
        case XPOWERS_AXP2101_CHG_PRE_STATE:
            return PowerService::ChargerState::PreCharge;
        case XPOWERS_AXP2101_CHG_CC_STATE:
            return PowerService::ChargerState::ConstantCurrent;
        case XPOWERS_AXP2101_CHG_CV_STATE:
            return PowerService::ChargerState::ConstantVoltage;
        case XPOWERS_AXP2101_CHG_DONE_STATE:
            return PowerService::ChargerState::Done;
        case XPOWERS_AXP2101_CHG_STOP_STATE:
            return PowerService::ChargerState::Stopped;
        default:
            return PowerService::ChargerState::Unknown;
    }
}

std::uint16_t configuredCurrentMa(std::uint8_t index) {
    // AXP2101 enum-to-current table used by the official XPowersLib example.
    static constexpr std::uint16_t TABLE[] = {
        0, 0, 0, 0, 100, 125, 150, 175, 200, 300, 400, 500, 600, 700, 800, 900, 1000
    };
    return index < (sizeof(TABLE) / sizeof(TABLE[0])) ? TABLE[index] : 0;
}

std::uint16_t targetVoltageMv(std::uint8_t index) {
    // AXP2101 enum-to-voltage table used by the official XPowersLib example.
    static constexpr std::uint16_t TABLE[] = {
        0, 4000, 4100, 4200, 4350, 4400, 0
    };
    return index < (sizeof(TABLE) / sizeof(TABLE[0])) ? TABLE[index] : 0;
}

std::uint16_t safeMillivolts(int value) {
    return value > 0 && value <= 65535 ? static_cast<std::uint16_t>(value) : 0U;
}

void copyText(char* destination, std::size_t capacity, const char* text) {
    if (destination == nullptr || capacity == 0U) {
        return;
    }
    std::snprintf(destination, capacity, "%s", text != nullptr ? text : "");
}

}  // namespace

bool PowerService::begin() {
    view_ = ViewState{};
    localizationRevision_ = App::Localization::revision();

    pmuReady = pmu.begin(
        Wire,
        AXP2101_SLAVE_ADDRESS,
        BoardPins::I2C_SDA,
        BoardPins::I2C_SCL);
    if (!pmuReady) {
        copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("AXP2101 nebyl nalezen", "AXP2101 was not detected"));
        LOG_E("POWER", "AXP2101 is unavailable on I2C");
        return false;
    }

    // The board has no battery-temperature sensor on the TS input. Keeping TS
    // detection disabled prevents false charge inhibition. Internal PMIC die
    // temperature measurement is enabled separately.
    pmu.disableTSPinMeasure();
    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
    pmu.enableVbusVoltageMeasure();
    pmu.enableSystemVoltageMeasure();
    pmu.enableTemperatureMeasure();

    view_.available = true;
    readState(true);
    LOG_I(
        "POWER",
        "AXP2101 ready: battery %u%%, %u mV, VBUS %s",
        static_cast<unsigned>(view_.batteryPercent),
        static_cast<unsigned>(view_.batteryVoltageMv),
        view_.vbusConnected ? "connected" : "disconnected");
    return true;
}

void PowerService::update(std::uint32_t now) {
    const std::uint32_t currentLocalizationRevision = App::Localization::revision();
    if (localizationRevision_ != currentLocalizationRevision) {
        localizationRevision_ = currentLocalizationRevision;
        if (pmuReady) {
            readState(true);
            lastPollAt_ = now;
        } else {
            copyText(
                view_.lastEvent,
                sizeof(view_.lastEvent),
                App::Localization::text(
                    "AXP2101 nebyl nalezen",
                    "AXP2101 was not detected"));
            ++view_.revision;
        }
        return;
    }

    if (!pmuReady || now - lastPollAt_ < AppConfig::POWER_POLL_INTERVAL_MS) {
        return;
    }
    lastPollAt_ = now;
    readState(false);
}

const PowerService::ViewState& PowerService::viewState() const {
    return view_;
}

void PowerService::readState(bool firstRead) {
    const ViewState previous = view_;

    view_.available = true;
    view_.batteryConnected = pmu.isBatteryConnect();
    view_.charging = pmu.isCharging();
    view_.discharging = pmu.isDischarge();
    view_.standby = pmu.isStandby();
    view_.vbusConnected = pmu.isVbusIn();
    view_.vbusGood = pmu.isVbusGood();
    view_.batteryVoltageMv = safeMillivolts(pmu.getBattVoltage());
    view_.vbusVoltageMv = safeMillivolts(pmu.getVbusVoltage());
    view_.systemVoltageMv = safeMillivolts(pmu.getSystemVoltage());

    const float rawTemperature = pmu.getTemperature();
    view_.pmicTemperatureValid = std::isfinite(rawTemperature) &&
        rawTemperature >= -40.0F && rawTemperature <= 150.0F;
    view_.pmicTemperatureC = view_.pmicTemperatureValid ? rawTemperature : 0.0F;

    const int rawPercent = view_.batteryConnected ? pmu.getBatteryPercent() : -1;
    view_.batteryPercentValid = rawPercent >= 0 && rawPercent <= 100;
    view_.batteryPercent = view_.batteryPercentValid
        ? static_cast<std::uint8_t>(rawPercent)
        : 0U;
    view_.chargerState = decodeChargerState(pmu.getChargerStatus());
    view_.configuredChargeCurrentMa = configuredCurrentMa(pmu.getChargerConstantCurr());
    view_.targetChargeVoltageMv = targetVoltageMv(pmu.getChargeTargetVoltage());
    const bool percentCritical = view_.batteryPercentValid &&
        view_.batteryPercent <= AppConfig::POWER_CRITICAL_PERCENT;
    const bool voltageCritical = view_.batteryVoltageMv > 0U &&
        view_.batteryVoltageMv <= AppConfig::POWER_CRITICAL_VOLTAGE_MV;
    view_.criticalBattery = view_.batteryConnected &&
        (percentCritical || voltageCritical);

    if (view_.charging) {
        copyText(view_.operatingText, sizeof(view_.operatingText), App::Localization::text("nabijeni", "charging"));
    } else if (view_.discharging) {
        copyText(view_.operatingText, sizeof(view_.operatingText), App::Localization::text("vybijeni", "discharging"));
    } else if (view_.vbusConnected) {
        copyText(view_.operatingText, sizeof(view_.operatingText), App::Localization::text("napajeni z USB-C", "powered by USB-C"));
    } else if (view_.standby) {
        copyText(view_.operatingText, sizeof(view_.operatingText), App::Localization::text("pohotovost", "standby"));
    } else {
        copyText(view_.operatingText, sizeof(view_.operatingText), App::Localization::text("neznamy stav", "unknown state"));
    }
    copyText(
        view_.chargerText,
        sizeof(view_.chargerText),
        chargerStateText(view_.chargerState));

    updateLastEvent(previous, firstRead);
    ++view_.revision;
}

void PowerService::updateLastEvent(const ViewState& previous, bool firstRead) {
    if (firstRead) {
        if (view_.vbusConnected) {
            copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("USB-C je pripojeno", "USB-C is connected"));
        } else if (view_.batteryConnected) {
            copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("Provoz z akumulatoru", "Running on battery"));
        } else {
            copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("Bez akumulatoru a USB-C", "No battery or USB-C power"));
        }
        return;
    }

    if (!previous.vbusConnected && view_.vbusConnected) {
        copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("Pripojeno USB-C", "USB-C connected"));
    } else if (previous.vbusConnected && !view_.vbusConnected) {
        copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("Odpojeno USB-C", "USB-C disconnected"));
    } else if (!previous.batteryConnected && view_.batteryConnected) {
        copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("Pripojen akumulator", "Battery connected"));
    } else if (previous.batteryConnected && !view_.batteryConnected) {
        copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("Odpojen akumulator", "Battery disconnected"));
    } else if (!previous.charging && view_.charging) {
        copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("Zahajeno nabijeni", "Charging started"));
    } else if (previous.charging && !view_.charging) {
        copyText(
            view_.lastEvent,
            sizeof(view_.lastEvent),
            view_.chargerState == ChargerState::Done
                ? App::Localization::text("Nabijeni dokonceno", "Charging complete")
                : App::Localization::text("Nabijeni preruseno", "Charging interrupted"));
    } else if (!previous.criticalBattery && view_.criticalBattery) {
        copyText(view_.lastEvent, sizeof(view_.lastEvent), App::Localization::text("Kriticky stav baterie", "Critical battery level"));
    }
}

}  // namespace Services
