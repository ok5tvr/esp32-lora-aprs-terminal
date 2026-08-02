#include "services/power_service.h"

#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

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

constexpr char POWER_HISTORY_NAMESPACE[] = "pwrhist";
constexpr char POWER_HISTORY_KEY[] = "history";
constexpr std::uint32_t POWER_HISTORY_MAGIC = 0x50575248U;  // PWRH
constexpr std::uint16_t POWER_HISTORY_STORAGE_VERSION = 1U;

struct PersistedPowerHistory {
    std::uint32_t magic = POWER_HISTORY_MAGIC;
    std::uint16_t version = POWER_HISTORY_STORAGE_VERSION;
    std::uint8_t count = 0;
    std::uint8_t reserved = 0;
    std::uint8_t percent[AppConfig::POWER_HISTORY_LENGTH] = {};
    std::uint8_t mode[AppConfig::POWER_HISTORY_LENGTH] = {};
    std::uint32_t atMinute[AppConfig::POWER_HISTORY_LENGTH] = {};
    std::uint32_t crc32 = 0;
};

static_assert(
    sizeof(PersistedPowerHistory) == 12U + (6U * AppConfig::POWER_HISTORY_LENGTH),
    "Unexpected persistent power-history layout");

std::uint32_t crc32(const std::uint8_t* data, std::size_t length) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (std::uint8_t bit = 0; bit < 8U; ++bit) {
            const std::uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

std::uint32_t historyCrc(const PersistedPowerHistory& history) {
    return crc32(
        reinterpret_cast<const std::uint8_t*>(&history),
        offsetof(PersistedPowerHistory, crc32));
}

bool validHistoryMode(std::uint8_t rawMode) {
    return rawMode <= static_cast<std::uint8_t>(PowerService::HistoryMode::Standby);
}

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
    lastPollAt_ = 0;
    lastHistorySampleAt_ = 0;
    historyClockAt_ = 0;
    historyClockRemainderMs_ = 0;
    historyMinuteNow_ = 0;
    lastHistoryMode_ = HistoryMode::Unknown;
    lastHistoryPercent_ = 0;
    pendingHistoryPercent_ = 0;
    pendingHistoryConfirmations_ = 0;
    historyStarted_ = false;
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

    const std::uint32_t now = millis();
    const bool historyRestored = loadPowerHistory(now);
    view_.available = true;
    readState(true);
    appendPowerHistory(now, !historyRestored);
    lastPollAt_ = now;
    LOG_I(
        "POWER",
        "AXP2101 ready: battery %u%%, %u mV, VBUS %s",
        static_cast<unsigned>(view_.batteryPercent),
        static_cast<unsigned>(view_.batteryVoltageMv),
        view_.vbusConnected ? "connected" : "disconnected");
    return true;
}

void PowerService::update(std::uint32_t now) {
    advanceHistoryClock(now);

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
    appendPowerHistory(now);
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


PowerService::HistoryMode PowerService::currentHistoryMode() const {
    if (view_.charging) {
        return HistoryMode::Charging;
    }
    if (view_.discharging) {
        return HistoryMode::Discharging;
    }
    if (view_.vbusConnected || view_.vbusGood) {
        return HistoryMode::UsbPower;
    }
    if (view_.standby) {
        return HistoryMode::Standby;
    }
    return HistoryMode::Unknown;
}

void PowerService::advanceHistoryClock(std::uint32_t now) {
    const std::uint32_t elapsedMs = now - historyClockAt_;
    historyClockAt_ = now;
    const std::uint64_t accumulatedMs =
        static_cast<std::uint64_t>(historyClockRemainderMs_) + elapsedMs;
    historyMinuteNow_ += static_cast<std::uint32_t>(accumulatedMs / 60000U);
    historyClockRemainderMs_ = static_cast<std::uint32_t>(accumulatedMs % 60000U);
}

bool PowerService::loadPowerHistory(std::uint32_t now) {
    historyClockAt_ = now;
    historyClockRemainderMs_ = 0U;
    historyMinuteNow_ = 0U;

    Preferences preferences;
    if (!preferences.begin(POWER_HISTORY_NAMESPACE, true)) {
        // Expected on the first boot before the namespace has been created.
        LOG_D("POWER", "No stored power history namespace");
        return false;
    }

    const std::size_t storedLength = preferences.getBytesLength(POWER_HISTORY_KEY);
    if (storedLength != sizeof(PersistedPowerHistory)) {
        preferences.end();
        return false;
    }

    PersistedPowerHistory stored;
    const std::size_t readLength = preferences.getBytes(
        POWER_HISTORY_KEY,
        &stored,
        sizeof(stored));
    preferences.end();
    if (readLength != sizeof(stored) ||
        stored.magic != POWER_HISTORY_MAGIC ||
        stored.version != POWER_HISTORY_STORAGE_VERSION ||
        stored.count == 0U ||
        stored.count > AppConfig::POWER_HISTORY_LENGTH ||
        stored.crc32 != historyCrc(stored)) {
        LOG_E("POWER", "Stored power history is invalid");
        return false;
    }

    for (std::size_t index = 0; index < stored.count; ++index) {
        if (stored.percent[index] > 100U || !validHistoryMode(stored.mode[index])) {
            LOG_E("POWER", "Stored power history contains invalid values");
            return false;
        }
        if (index > 0U && stored.atMinute[index] < stored.atMinute[index - 1U]) {
            LOG_E("POWER", "Stored power history time order is invalid");
            return false;
        }
    }

    const std::uint32_t firstStoredMinute = stored.atMinute[0];
    view_.powerHistoryCount = stored.count;
    for (std::size_t index = 0; index < stored.count; ++index) {
        view_.powerHistoryPercent[index] = stored.percent[index];
        view_.powerHistoryMode[index] = static_cast<HistoryMode>(stored.mode[index]);
        // Normalize after every reboot. Only the relative span is relevant to
        // the graph and this prevents the logical minute counter from growing
        // without limit over many years of operation.
        view_.powerHistoryAtMinute[index] = stored.atMinute[index] - firstStoredMinute;
    }
    ++view_.powerHistoryRevision;

    const std::size_t last = stored.count - 1U;
    lastHistoryPercent_ = stored.percent[last];
    lastHistoryMode_ = static_cast<HistoryMode>(stored.mode[last]);
    pendingHistoryPercent_ = 0U;
    pendingHistoryConfirmations_ = 0U;
    lastHistorySampleAt_ = now;
    historyMinuteNow_ = view_.powerHistoryAtMinute[last] + 1U;
    historyStarted_ = true;

    LOG_I(
        "POWER",
        "Restored %u power history points",
        static_cast<unsigned>(stored.count));
    return true;
}

bool PowerService::savePowerHistory() const {
    PersistedPowerHistory stored;
    stored.count = view_.powerHistoryCount;
    for (std::size_t index = 0; index < view_.powerHistoryCount; ++index) {
        stored.percent[index] = view_.powerHistoryPercent[index];
        stored.mode[index] = static_cast<std::uint8_t>(view_.powerHistoryMode[index]);
        stored.atMinute[index] = view_.powerHistoryAtMinute[index];
    }
    stored.crc32 = historyCrc(stored);

    Preferences preferences;
    if (!preferences.begin(POWER_HISTORY_NAMESPACE, false)) {
        LOG_E("POWER", "Power history NVS write open failed");
        return false;
    }
    const std::size_t written = preferences.putBytes(
        POWER_HISTORY_KEY,
        &stored,
        sizeof(stored));
    preferences.end();
    if (written != sizeof(stored)) {
        LOG_E("POWER", "Power history NVS write failed");
        return false;
    }
    return true;
}

void PowerService::appendPowerHistory(std::uint32_t now, bool force) {
    if (!view_.available || !view_.batteryConnected ||
        !view_.batteryPercentValid || view_.batteryPercent > 100U) {
        // Force an immediate point when a valid battery reading returns after
        // disconnection or a temporarily unavailable fuel-gauge value.
        historyStarted_ = false;
        lastHistoryMode_ = HistoryMode::Unknown;
        pendingHistoryConfirmations_ = 0U;
        return;
    }

    const HistoryMode mode = currentHistoryMode();
    const bool modeChanged = historyStarted_ && mode != lastHistoryMode_;
    const int percentDelta = static_cast<int>(view_.batteryPercent) -
        static_cast<int>(lastHistoryPercent_);
    const bool percentOutsideStep = historyStarted_ &&
        (percentDelta >= static_cast<int>(AppConfig::POWER_HISTORY_PERCENT_STEP) ||
         percentDelta <= -static_cast<int>(AppConfig::POWER_HISTORY_PERCENT_STEP));
    if (percentOutsideStep) {
        if (pendingHistoryConfirmations_ > 0U &&
            pendingHistoryPercent_ == view_.batteryPercent) {
            if (pendingHistoryConfirmations_ < 255U) {
                ++pendingHistoryConfirmations_;
            }
        } else {
            pendingHistoryPercent_ = view_.batteryPercent;
            pendingHistoryConfirmations_ = 1U;
        }
    } else {
        pendingHistoryConfirmations_ = 0U;
    }
    const bool percentChanged = percentOutsideStep &&
        pendingHistoryConfirmations_ >= AppConfig::POWER_HISTORY_PERCENT_CONFIRMATIONS;
    const bool intervalElapsed = historyStarted_ &&
        now - lastHistorySampleAt_ >= AppConfig::POWER_HISTORY_MAX_INTERVAL_MS;
    if (!force && historyStarted_ && !modeChanged && !percentChanged && !intervalElapsed) {
        return;
    }

    const std::size_t capacity = AppConfig::POWER_HISTORY_LENGTH;
    std::size_t count = view_.powerHistoryCount;
    advanceHistoryClock(now);
    const std::uint32_t pointMinute = historyMinuteNow_;
    if (count < capacity) {
        view_.powerHistoryPercent[count] = view_.batteryPercent;
        view_.powerHistoryMode[count] = mode;
        view_.powerHistoryAtMinute[count] = pointMinute;
        ++count;
    } else {
        std::memmove(
            &view_.powerHistoryPercent[0],
            &view_.powerHistoryPercent[1],
            (capacity - 1U) * sizeof(view_.powerHistoryPercent[0]));
        std::memmove(
            &view_.powerHistoryMode[0],
            &view_.powerHistoryMode[1],
            (capacity - 1U) * sizeof(view_.powerHistoryMode[0]));
        std::memmove(
            &view_.powerHistoryAtMinute[0],
            &view_.powerHistoryAtMinute[1],
            (capacity - 1U) * sizeof(view_.powerHistoryAtMinute[0]));
        view_.powerHistoryPercent[capacity - 1U] = view_.batteryPercent;
        view_.powerHistoryMode[capacity - 1U] = mode;
        view_.powerHistoryAtMinute[capacity - 1U] = pointMinute;
        count = capacity;
    }

    view_.powerHistoryCount = static_cast<std::uint8_t>(count);
    ++view_.powerHistoryRevision;
    ++view_.revision;
    lastHistorySampleAt_ = now;
    lastHistoryMode_ = mode;
    lastHistoryPercent_ = view_.batteryPercent;
    pendingHistoryConfirmations_ = 0U;
    historyStarted_ = true;
    savePowerHistory();

    LOG_D(
        "POWER",
        "History %u%% mode %u minute %lu (%u/%u)",
        static_cast<unsigned>(view_.batteryPercent),
        static_cast<unsigned>(mode),
        static_cast<unsigned long>(pointMinute),
        static_cast<unsigned>(count),
        static_cast<unsigned>(capacity));
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
