#include "services/settings_service.h"

#include <Arduino.h>
#include <Preferences.h>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "app_config.h"
#include "app/tracker_symbols.h"
#include "app/localization.h"
#include "app_log.h"

namespace Services {
namespace {

constexpr char NVS_NAMESPACE[] = "loraaprs";
constexpr char KEY_CALLSIGN[] = "call";
constexpr char KEY_LATITUDE[] = "lat";
constexpr char KEY_LONGITUDE[] = "lon";
constexpr char KEY_TRACKER_ENABLED[] = "trken";
constexpr char KEY_TRAIL_ENABLED[] = "trailen";
constexpr char KEY_TRACKER_SOURCE[] = "trksrc";
constexpr char KEY_TRACKER_FORMAT[] = "trkfmt";
constexpr char KEY_TRACKER_MODE[] = "trkmode";
constexpr char KEY_TRACKER_SYMBOL[] = "trksym";
constexpr char KEY_TRACKER_INTERVAL[] = "trkint";
constexpr char KEY_DIGI_ENABLED[] = "digien";
constexpr char KEY_DIGI_MODE[] = "digimode";
constexpr char KEY_DIGI_MAX[] = "digimax";
constexpr char KEY_IGATE_ENABLED[] = "igaten";
constexpr char KEY_WIFI_SSID[] = "wssid";
constexpr char KEY_WIFI_PASSWORD[] = "wpass";
constexpr char KEY_APRS_IS_SERVER[] = "ishost";
constexpr char KEY_APRS_IS_PORT[] = "isport";
constexpr char KEY_APRS_IS_PASSCODE[] = "ispass";
constexpr char KEY_APRS_IS_FILTER[] = "isfilter";
constexpr char KEY_BATTERY_BRIGHTNESS[] = "batbright";
constexpr char KEY_DISPLAY_TIMEOUT[] = "dispto";
constexpr char KEY_TRACKER_LANGUAGE[] = "trklang";
constexpr char KEY_LORA_PRESET[] = "lprof";
constexpr char KEY_LORA_FREQUENCY[] = "lfreq";
constexpr char KEY_LORA_BANDWIDTH[] = "lbw";
constexpr char KEY_LORA_SF[] = "lsf";
constexpr char KEY_LORA_CR[] = "lcr";
constexpr char KEY_LORA_POWER[] = "lpwr";

bool validSource(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(App::TrackerPositionSource::DefaultPosition);
}

bool validFormat(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(App::TrackerPositionFormat::Compressed);
}

bool validMode(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(App::TrackerBeaconMode::SmartBeacon);
}


bool validDigiMode(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(App::DigiMode::FillInAndWide2);
}

bool validDisplayTimeout(std::uint16_t value) {
    return value == 0U || value == 30U || value == 60U ||
        value == 120U || value == 300U;
}

bool validLoRaPreset(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(App::LoRaPreset::Custom);
}

bool validUiLanguage(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(App::UiLanguage::English);
}

LoRaProfile::Config makeLoRaConfig(
    float frequencyMHz,
    float bandwidthKHz,
    std::uint8_t spreadingFactor,
    std::uint8_t codingRate,
    std::int8_t outputPowerDbm) {

    LoRaProfile::Config config;
    config.frequencyMHz = frequencyMHz;
    config.bandwidthKHz = bandwidthKHz;
    config.spreadingFactor = spreadingFactor;
    config.codingRate = codingRate;
    config.outputPowerDbm = outputPowerDbm;
    return config;
}

bool validAprsIsRfCallsign(const char* callsign) {
    if (callsign == nullptr) {
        return false;
    }
    const std::size_t length = std::strlen(callsign);
    if (length < 3 || length > 9) {
        return false;
    }
    const char* dash = std::strchr(callsign, '-');
    const std::size_t baseLength = dash == nullptr
        ? length
        : static_cast<std::size_t>(dash - callsign);
    if (baseLength < 3 || baseLength > 6) {
        return false;
    }
    // APRS-IS treats a missing SSID as zero; an explicit -0 must not be used.
    return dash == nullptr || std::strcmp(dash + 1, "0") != 0;
}

bool copyPrintable(
    const char* input,
    char* output,
    std::size_t outputCapacity,
    bool allowEmpty) {

    if (input == nullptr || output == nullptr || outputCapacity == 0) {
        return false;
    }
    std::size_t length = std::strlen(input);
    while (length > 0 && (input[length - 1] == ' ' || input[length - 1] == '\t')) {
        --length;
    }
    std::size_t begin = 0;
    while (begin < length && (input[begin] == ' ' || input[begin] == '\t')) {
        ++begin;
    }
    length -= begin;
    if ((!allowEmpty && length == 0) || length + 1 > outputCapacity) {
        return false;
    }
    for (std::size_t index = 0; index < length; ++index) {
        const unsigned char value = static_cast<unsigned char>(input[begin + index]);
        if (value < 32 || value > 126 || value == '\r' || value == '\n') {
            return false;
        }
    }
    if (length > 0) {
        std::memcpy(output, input + begin, length);
    }
    output[length] = '\0';
    return true;
}

bool normalizeServer(const char* input, char* output, std::size_t capacity) {
    if (!copyPrintable(input, output, capacity, false)) {
        return false;
    }
    for (const char* cursor = output; *cursor != '\0'; ++cursor) {
        const char value = *cursor;
        if (!((value >= 'A' && value <= 'Z') ||
              (value >= 'a' && value <= 'z') ||
              (value >= '0' && value <= '9') || value == '.' || value == '-')) {
            return false;
        }
    }
    return true;
}


bool saveOptionalString(Preferences& preferences, const char* key, const char* value) {
    if (value == nullptr || value[0] == '\0') {
        if (preferences.isKey(key)) {
            return preferences.remove(key);
        }
        return true;
    }
    return preferences.putString(key, value) > 0;
}

bool normalizeFilter(const char* input, char* output, std::size_t capacity) {
    char temporary[SettingsService::APRS_IS_FILTER_CAPACITY] = {};
    if (!copyPrintable(input != nullptr ? input : "", temporary, sizeof(temporary), true)) {
        return false;
    }
    const char* value = temporary;
    const char prefix[] = "filter ";
    bool hasPrefix = true;
    for (std::size_t index = 0; index < sizeof(prefix) - 1; ++index) {
        char left = value[index];
        char right = prefix[index];
        if (left >= 'A' && left <= 'Z') {
            left = static_cast<char>(left - 'A' + 'a');
        }
        if (left != right) {
            hasPrefix = false;
            break;
        }
    }
    if (hasPrefix) {
        value += sizeof(prefix) - 1;
    }
    return copyPrintable(value, output, capacity, true);
}

}  // namespace

bool SettingsService::begin() {
    view_ = ViewState{};
    std::strncpy(view_.callsign, AppConfig::DEFAULT_CALLSIGN, sizeof(view_.callsign) - 1);
    view_.defaultLatitude = AppConfig::DEFAULT_LATITUDE;
    view_.defaultLongitude = AppConfig::DEFAULT_LONGITUDE;
    view_.batteryBrightnessPercent = AppConfig::DISPLAY_DEFAULT_BATTERY_BRIGHTNESS_PERCENT;
    view_.displayTimeoutSeconds = AppConfig::DISPLAY_DEFAULT_TIMEOUT_SECONDS;
    view_.loraPreset = App::LoRaPreset::CzeAprs;
    view_.loraFrequencyMHz = LoRaProfile::FREQUENCY_MHZ;
    view_.loraBandwidthKHz = LoRaProfile::BANDWIDTH_KHZ;
    view_.loraSpreadingFactor = LoRaProfile::SPREADING_FACTOR;
    view_.loraCodingRate = LoRaProfile::CODING_RATE;
    view_.loraOutputPowerDbm = LoRaProfile::OUTPUT_POWER_DBM;
    view_.trackerFixedIntervalSeconds = AppConfig::TRACKER_DEFAULT_INTERVAL_SECONDS;
    view_.digiMaxWideHops = 2;
    std::strncpy(
        view_.aprsIsServer,
        AppConfig::DEFAULT_APRS_IS_SERVER,
        sizeof(view_.aprsIsServer) - 1);
    view_.aprsIsPort = AppConfig::DEFAULT_APRS_IS_PORT;
    std::strncpy(
        view_.aprsIsFilter,
        AppConfig::DEFAULT_APRS_IS_FILTER,
        sizeof(view_.aprsIsFilter) - 1);

    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        LOG_E("SETTINGS", "NVS open failed; defaults remain active");
        return false;
    }

    const String storedCallsign = preferences.getString(KEY_CALLSIGN, AppConfig::DEFAULT_CALLSIGN);
    char normalized[CALLSIGN_CAPACITY] = {};
    if (normalizeCallsign(storedCallsign.c_str(), normalized, sizeof(normalized))) {
        std::strncpy(view_.callsign, normalized, sizeof(view_.callsign) - 1);
    }

    const double latitude = preferences.getDouble(KEY_LATITUDE, AppConfig::DEFAULT_LATITUDE);
    const double longitude = preferences.getDouble(KEY_LONGITUDE, AppConfig::DEFAULT_LONGITUDE);
    if (std::isfinite(latitude) && latitude >= -90.0 && latitude <= 90.0) {
        view_.defaultLatitude = latitude;
    }
    if (std::isfinite(longitude) && longitude >= -180.0 && longitude <= 180.0) {
        view_.defaultLongitude = longitude;
    }

    const std::uint8_t storedBrightness = preferences.getUChar(
        KEY_BATTERY_BRIGHTNESS,
        AppConfig::DISPLAY_DEFAULT_BATTERY_BRIGHTNESS_PERCENT);
    if (storedBrightness >= AppConfig::DISPLAY_MIN_BATTERY_BRIGHTNESS_PERCENT &&
        storedBrightness <= AppConfig::DISPLAY_MAX_BATTERY_BRIGHTNESS_PERCENT) {
        view_.batteryBrightnessPercent = storedBrightness;
    }
    const std::uint32_t storedDisplayTimeout = preferences.getUInt(
        KEY_DISPLAY_TIMEOUT,
        AppConfig::DISPLAY_DEFAULT_TIMEOUT_SECONDS);
    if (storedDisplayTimeout <= 65535U &&
        validDisplayTimeout(static_cast<std::uint16_t>(storedDisplayTimeout))) {
        view_.displayTimeoutSeconds = static_cast<std::uint16_t>(storedDisplayTimeout);
    }
    const std::uint8_t storedUiLanguage = preferences.getUChar(
        KEY_TRACKER_LANGUAGE,
        static_cast<std::uint8_t>(App::UiLanguage::Czech));
    if (validUiLanguage(storedUiLanguage)) {
        view_.uiLanguage = static_cast<App::UiLanguage>(storedUiLanguage);
    }

    const std::uint8_t storedLoRaPreset = preferences.getUChar(
        KEY_LORA_PRESET,
        static_cast<std::uint8_t>(App::LoRaPreset::CzeAprs));
    if (validLoRaPreset(storedLoRaPreset)) {
        view_.loraPreset = static_cast<App::LoRaPreset>(storedLoRaPreset);
    }
    if (view_.loraPreset == App::LoRaPreset::Custom) {
        const float storedFrequency = preferences.getFloat(
            KEY_LORA_FREQUENCY, LoRaProfile::FREQUENCY_MHZ);
        const float storedBandwidth = preferences.getFloat(
            KEY_LORA_BANDWIDTH, LoRaProfile::BANDWIDTH_KHZ);
        const std::uint8_t storedSf = preferences.getUChar(
            KEY_LORA_SF, LoRaProfile::SPREADING_FACTOR);
        const std::uint8_t storedCr = preferences.getUChar(
            KEY_LORA_CR, LoRaProfile::CODING_RATE);
        const std::int32_t storedPower = preferences.getInt(
            KEY_LORA_POWER, LoRaProfile::OUTPUT_POWER_DBM);
        const LoRaProfile::Config storedConfig = makeLoRaConfig(
            storedFrequency,
            storedBandwidth,
            storedSf,
            storedCr,
            static_cast<std::int8_t>(storedPower));
        if (storedPower >= LoRaProfile::MIN_OUTPUT_POWER_DBM &&
            storedPower <= LoRaProfile::MAX_OUTPUT_POWER_DBM &&
            LoRaProfile::isValidConfig(storedConfig)) {
            view_.loraFrequencyMHz = storedConfig.frequencyMHz;
            view_.loraBandwidthKHz = storedConfig.bandwidthKHz;
            view_.loraSpreadingFactor = storedConfig.spreadingFactor;
            view_.loraCodingRate = storedConfig.codingRate;
            view_.loraOutputPowerDbm = storedConfig.outputPowerDbm;
        } else {
            view_.loraPreset = App::LoRaPreset::CzeAprs;
        }
    }

    view_.trackerEnabled = preferences.getBool(KEY_TRACKER_ENABLED, false);
    view_.trailEnabled = preferences.getBool(KEY_TRAIL_ENABLED, false);
    const std::uint8_t sourceValue = preferences.getUChar(
        KEY_TRACKER_SOURCE,
        static_cast<std::uint8_t>(App::TrackerPositionSource::Gps));
    const std::uint8_t formatValue = preferences.getUChar(
        KEY_TRACKER_FORMAT,
        static_cast<std::uint8_t>(App::TrackerPositionFormat::Uncompressed));
    const std::uint8_t modeValue = preferences.getUChar(
        KEY_TRACKER_MODE,
        static_cast<std::uint8_t>(App::TrackerBeaconMode::FixedInterval));
    const std::uint8_t symbolValue = preferences.getUChar(
        KEY_TRACKER_SYMBOL,
        static_cast<std::uint8_t>(App::TrackerSymbol::Car));
    const std::uint32_t interval = preferences.getUInt(
        KEY_TRACKER_INTERVAL,
        AppConfig::TRACKER_DEFAULT_INTERVAL_SECONDS);

    if (validSource(sourceValue)) {
        view_.trackerSource = static_cast<App::TrackerPositionSource>(sourceValue);
    }
    if (validFormat(formatValue)) {
        view_.trackerFormat = static_cast<App::TrackerPositionFormat>(formatValue);
    }
    if (validMode(modeValue)) {
        view_.trackerMode = static_cast<App::TrackerBeaconMode>(modeValue);
    }
    const App::TrackerSymbol storedSymbol = static_cast<App::TrackerSymbol>(symbolValue);
    if (App::validTrackerSymbol(storedSymbol)) {
        view_.trackerSymbol = storedSymbol;
    }
    if (interval >= AppConfig::TRACKER_MIN_INTERVAL_SECONDS &&
        interval <= AppConfig::TRACKER_MAX_INTERVAL_SECONDS) {
        view_.trackerFixedIntervalSeconds = interval;
    }

    // A static default position has no speed/course information, so it cannot
    // drive the SmartBeacon algorithm. Fall back safely if old NVS data
    // contains that impossible combination.
    if (view_.trackerSource == App::TrackerPositionSource::DefaultPosition &&
        view_.trackerMode == App::TrackerBeaconMode::SmartBeacon) {
        view_.trackerMode = App::TrackerBeaconMode::FixedInterval;
    }

    view_.digiEnabled = preferences.getBool(KEY_DIGI_ENABLED, false);
    const std::uint8_t digiModeValue = preferences.getUChar(
        KEY_DIGI_MODE,
        static_cast<std::uint8_t>(App::DigiMode::FillInWide1));
    if (validDigiMode(digiModeValue)) {
        view_.digiMode = static_cast<App::DigiMode>(digiModeValue);
    }
    const std::uint8_t maxWide = preferences.getUChar(KEY_DIGI_MAX, 2);
    if (maxWide >= 1 && maxWide <= 2) {
        view_.digiMaxWideHops = maxWide;
    }

    view_.igateEnabled = preferences.getBool(KEY_IGATE_ENABLED, false);
    const String storedSsid = preferences.getString(KEY_WIFI_SSID, "");
    const String storedPassword = preferences.getString(KEY_WIFI_PASSWORD, "");
    const String storedServer = preferences.getString(
        KEY_APRS_IS_SERVER,
        AppConfig::DEFAULT_APRS_IS_SERVER);
    const String storedFilter = preferences.getString(
        KEY_APRS_IS_FILTER,
        AppConfig::DEFAULT_APRS_IS_FILTER);
    copyPrintable(
        storedSsid.c_str(), view_.wifiSsid, sizeof(view_.wifiSsid), true);
    copyPrintable(
        storedPassword.c_str(), view_.wifiPassword, sizeof(view_.wifiPassword), true);
    if (!normalizeServer(
            storedServer.c_str(), view_.aprsIsServer, sizeof(view_.aprsIsServer))) {
        std::strncpy(
            view_.aprsIsServer,
            AppConfig::DEFAULT_APRS_IS_SERVER,
            sizeof(view_.aprsIsServer) - 1);
    }
    normalizeFilter(
        storedFilter.c_str(), view_.aprsIsFilter, sizeof(view_.aprsIsFilter));
    const std::uint32_t storedPort = preferences.getUInt(
        KEY_APRS_IS_PORT,
        AppConfig::DEFAULT_APRS_IS_PORT);
    if (storedPort >= 1 && storedPort <= 65535) {
        view_.aprsIsPort = static_cast<std::uint16_t>(storedPort);
    }
    const std::int32_t storedPasscode = preferences.getInt(KEY_APRS_IS_PASSCODE, -1);
    if (storedPasscode >= -1 && storedPasscode <= 32767) {
        view_.aprsIsPasscode = storedPasscode;
    }
    if (view_.igateEnabled &&
        (view_.wifiSsid[0] == '\0' || view_.aprsIsPasscode < 0 ||
         !validAprsIsRfCallsign(view_.callsign))) {
        // Never start an unverified or unconfigured IGate after boot.
        view_.igateEnabled = false;
    }

    preferences.end();
    view_.persistentStorageReady = true;
    LOG_I(
        "SETTINGS",
        "Loaded CALL %s, default position %.6f %.6f, tracker %s",
        view_.callsign,
        view_.defaultLatitude,
        view_.defaultLongitude,
        view_.trackerEnabled ? "enabled" : "disabled");
    return true;
}

bool SettingsService::save(
    const char* callsign,
    double latitude,
    double longitude,
    std::uint8_t batteryBrightnessPercent,
    std::uint16_t displayTimeoutSeconds,
    App::UiLanguage uiLanguage,
    bool otaEnabled,
    App::LoRaPreset loraPreset,
    float loraFrequencyMHz,
    float loraBandwidthKHz,
    std::uint8_t loraSpreadingFactor,
    std::uint8_t loraCodingRate,
    std::int8_t loraOutputPowerDbm,
    char* errorText,
    std::size_t errorTextCapacity) {

    const bool english = uiLanguage == App::UiLanguage::English;
    char normalized[CALLSIGN_CAPACITY] = {};
    if (!normalizeCallsign(callsign, normalized, sizeof(normalized))) {
        setError(errorText, errorTextCapacity, english ? "Invalid callsign. Use 1-6 characters and optional SSID 0-15." : "Neplatny CALL. Pouzijte 1-6 znaku a volitelne SSID 0-15.");
        return false;
    }
    if (!std::isfinite(latitude) || latitude < -90.0 || latitude > 90.0) {
        setError(errorText, errorTextCapacity, english ? "Latitude must be between -90 and 90." : "Zemepisna sirka musi byt -90 az 90.");
        return false;
    }
    if (!std::isfinite(longitude) || longitude < -180.0 || longitude > 180.0) {
        setError(errorText, errorTextCapacity, english ? "Longitude must be between -180 and 180." : "Zemepisna delka musi byt -180 az 180.");
        return false;
    }
    if (batteryBrightnessPercent < AppConfig::DISPLAY_MIN_BATTERY_BRIGHTNESS_PERCENT ||
        batteryBrightnessPercent > AppConfig::DISPLAY_MAX_BATTERY_BRIGHTNESS_PERCENT) {
        setError(errorText, errorTextCapacity, english ? "Battery brightness must be between 10 and 100 percent." : "Jas baterie musi byt 10 az 100 procent.");
        return false;
    }
    if (!validDisplayTimeout(displayTimeoutSeconds)) {
        setError(errorText, errorTextCapacity, english ? "Invalid display timeout." : "Neplatny cas vypnuti displeje.");
        return false;
    }
    if (!validUiLanguage(static_cast<std::uint8_t>(uiLanguage))) {
        setError(errorText, errorTextCapacity, english ? "Invalid user interface language." : "Neplatny jazyk rozhrani.");
        return false;
    }
    if (loraPreset != App::LoRaPreset::CzeAprs &&
        loraPreset != App::LoRaPreset::Custom) {
        setError(errorText, errorTextCapacity, english ? "Invalid LoRa profile." : "Neplatny profil LoRa.");
        return false;
    }

    LoRaProfile::Config loraConfig = LoRaProfile::czeAprsConfig();
    if (loraPreset == App::LoRaPreset::Custom) {
        loraConfig = makeLoRaConfig(
            loraFrequencyMHz,
            loraBandwidthKHz,
            loraSpreadingFactor,
            loraCodingRate,
            loraOutputPowerDbm);
        if (!LoRaProfile::isValidConfig(loraConfig)) {
            setError(
                errorText,
                errorTextCapacity,
                english ? "LoRa: frequency 410-525 MHz, SF7-12, CR 4/5-4/8, power 2-17 dBm." : "LoRa: frekvence 410-525 MHz, SF7-12, CR 4/5-4/8, vykon 2-17 dBm.");
            return false;
        }
    }

    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        setError(errorText, errorTextCapacity, english ? "Cannot open NVS for writing." : "Nelze otevrit NVS pro zapis.");
        return false;
    }

    const bool callSaved = preferences.putString(KEY_CALLSIGN, normalized) > 0;
    const bool latSaved = preferences.putDouble(KEY_LATITUDE, latitude) > 0;
    const bool lonSaved = preferences.putDouble(KEY_LONGITUDE, longitude) > 0;
    const bool brightnessSaved = preferences.putUChar(
        KEY_BATTERY_BRIGHTNESS,
        batteryBrightnessPercent) > 0;
    const bool timeoutSaved = preferences.putUInt(
        KEY_DISPLAY_TIMEOUT,
        displayTimeoutSeconds) > 0;
    const bool uiLanguageSaved = preferences.putUChar(
        KEY_TRACKER_LANGUAGE,
        static_cast<std::uint8_t>(uiLanguage)) > 0;
    const bool loraPresetSaved = preferences.putUChar(
        KEY_LORA_PRESET,
        static_cast<std::uint8_t>(loraPreset)) > 0;
    const bool loraFrequencySaved = preferences.putFloat(
        KEY_LORA_FREQUENCY, loraConfig.frequencyMHz) > 0;
    const bool loraBandwidthSaved = preferences.putFloat(
        KEY_LORA_BANDWIDTH, loraConfig.bandwidthKHz) > 0;
    const bool loraSfSaved = preferences.putUChar(
        KEY_LORA_SF, loraConfig.spreadingFactor) > 0;
    const bool loraCrSaved = preferences.putUChar(
        KEY_LORA_CR, loraConfig.codingRate) > 0;
    const bool loraPowerSaved = preferences.putInt(
        KEY_LORA_POWER, loraConfig.outputPowerDbm) > 0;
    preferences.end();

    if (!callSaved || !latSaved || !lonSaved || !brightnessSaved || !timeoutSaved ||
        !uiLanguageSaved || !loraPresetSaved || !loraFrequencySaved || !loraBandwidthSaved ||
        !loraSfSaved || !loraCrSaved || !loraPowerSaved) {
        setError(errorText, errorTextCapacity, english ? "Saving settings to NVS failed." : "Ulozeni do NVS se nepodarilo.");
        return false;
    }

    std::strncpy(view_.callsign, normalized, sizeof(view_.callsign) - 1);
    view_.callsign[sizeof(view_.callsign) - 1] = '\0';
    view_.defaultLatitude = latitude;
    view_.defaultLongitude = longitude;
    view_.batteryBrightnessPercent = batteryBrightnessPercent;
    view_.displayTimeoutSeconds = displayTimeoutSeconds;
    view_.uiLanguage = uiLanguage;
    // OTA is a deliberate maintenance mode. It is kept only in RAM so a
    // successful update or any reboot always returns the device to normal mode.
    view_.otaEnabled = otaEnabled;
    view_.loraPreset = loraPreset;
    view_.loraFrequencyMHz = loraConfig.frequencyMHz;
    view_.loraBandwidthKHz = loraConfig.bandwidthKHz;
    view_.loraSpreadingFactor = loraConfig.spreadingFactor;
    view_.loraCodingRate = loraConfig.codingRate;
    view_.loraOutputPowerDbm = loraConfig.outputPowerDbm;
    view_.persistentStorageReady = true;
    ++view_.revision;
    setError(errorText, errorTextCapacity, "");
    LOG_I(
        "SETTINGS",
        "Saved CALL %s, position %.6f %.6f, display %u%%/%u s, UI language %u, OTA %s, LoRa %.3f MHz BW %.1f SF%u CR4/%u P%d",
        view_.callsign,
        view_.defaultLatitude,
        view_.defaultLongitude,
        static_cast<unsigned>(view_.batteryBrightnessPercent),
        static_cast<unsigned>(view_.displayTimeoutSeconds),
        static_cast<unsigned>(view_.uiLanguage),
        view_.otaEnabled ? "enabled" : "disabled",
        static_cast<double>(view_.loraFrequencyMHz),
        static_cast<double>(view_.loraBandwidthKHz),
        static_cast<unsigned>(view_.loraSpreadingFactor),
        static_cast<unsigned>(view_.loraCodingRate),
        static_cast<int>(view_.loraOutputPowerDbm));
    return true;
}

bool SettingsService::saveTracker(
    bool enabled,
    bool trailEnabled,
    App::TrackerPositionSource source,
    App::TrackerPositionFormat format,
    App::TrackerBeaconMode mode,
    App::TrackerSymbol symbol,
    std::uint32_t fixedIntervalSeconds,
    char* errorText,
    std::size_t errorTextCapacity) {

    const bool english = view_.uiLanguage == App::UiLanguage::English;
    if (fixedIntervalSeconds < AppConfig::TRACKER_MIN_INTERVAL_SECONDS ||
        fixedIntervalSeconds > AppConfig::TRACKER_MAX_INTERVAL_SECONDS) {
        setError(
            errorText,
            errorTextCapacity,
            english
                ? "Interval must be between 30 and 3600 seconds."
                : "Interval musi byt 30 az 3600 sekund.");
        return false;
    }
    if (!App::validTrackerSymbol(symbol)) {
        setError(
            errorText,
            errorTextCapacity,
            english ? "Invalid tracker APRS symbol." : "Neplatny APRS symbol trackeru.");
        return false;
    }
    if (source == App::TrackerPositionSource::DefaultPosition &&
        mode == App::TrackerBeaconMode::SmartBeacon) {
        setError(
            errorText,
            errorTextCapacity,
            english
                ? "SmartBeacon requires the GPS position source."
                : "SmartBeacon vyzaduje zdroj GPS.");
        return false;
    }

    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        setError(
            errorText,
            errorTextCapacity,
            english ? "Cannot open NVS for writing." : "Nelze otevrit NVS pro zapis.");
        return false;
    }

    const bool enabledSaved = preferences.putBool(KEY_TRACKER_ENABLED, enabled) > 0;
    const bool trailEnabledSaved = preferences.putBool(KEY_TRAIL_ENABLED, trailEnabled) > 0;
    const bool sourceSaved = preferences.putUChar(
        KEY_TRACKER_SOURCE,
        static_cast<std::uint8_t>(source)) > 0;
    const bool formatSaved = preferences.putUChar(
        KEY_TRACKER_FORMAT,
        static_cast<std::uint8_t>(format)) > 0;
    const bool modeSaved = preferences.putUChar(
        KEY_TRACKER_MODE,
        static_cast<std::uint8_t>(mode)) > 0;
    const bool symbolSaved = preferences.putUChar(
        KEY_TRACKER_SYMBOL,
        static_cast<std::uint8_t>(symbol)) > 0;
    const bool intervalSaved = preferences.putUInt(
        KEY_TRACKER_INTERVAL,
        fixedIntervalSeconds) > 0;
    preferences.end();

    if (!enabledSaved || !trailEnabledSaved || !sourceSaved || !formatSaved || !modeSaved ||
        !symbolSaved || !intervalSaved) {
        setError(
            errorText,
            errorTextCapacity,
            english
                ? "Saving tracker settings to NVS failed."
                : "Ulozeni trackeru do NVS se nepodarilo.");
        return false;
    }

    view_.trackerEnabled = enabled;
    view_.trailEnabled = trailEnabled;
    view_.trackerSource = source;
    view_.trackerFormat = format;
    view_.trackerMode = mode;
    view_.trackerSymbol = symbol;
    view_.trackerFixedIntervalSeconds = fixedIntervalSeconds;
    view_.persistentStorageReady = true;
    ++view_.revision;
    setError(errorText, errorTextCapacity, "");
    LOG_I(
        "SETTINGS",
        "Tracker saved: %s trail=%s source=%u format=%u mode=%u symbol=%u interval=%u",
        enabled ? "enabled" : "disabled",
        trailEnabled ? "enabled" : "disabled",
        static_cast<unsigned>(source),
        static_cast<unsigned>(format),
        static_cast<unsigned>(mode),
        static_cast<unsigned>(symbol),
        static_cast<unsigned>(fixedIntervalSeconds));
    return true;
}

bool SettingsService::saveDigiIgate(
    bool digiEnabled,
    App::DigiMode digiMode,
    std::uint8_t maxWideHops,
    bool igateEnabled,
    const char* wifiSsid,
    const char* wifiPassword,
    const char* aprsIsServer,
    std::uint16_t aprsIsPort,
    std::int32_t aprsIsPasscode,
    const char* aprsIsFilter,
    char* errorText,
    std::size_t errorTextCapacity) {

    if (!validDigiMode(static_cast<std::uint8_t>(digiMode))) {
        setError(errorText, errorTextCapacity, App::Localization::text("Neplatny rezim digipeateru.", "Invalid digipeater mode."));
        return false;
    }
    if (maxWideHops < 1 || maxWideHops > 2) {
        setError(errorText, errorTextCapacity, App::Localization::text("Maximalni WIDE musi byt 1 nebo 2 hop.", "Maximum WIDE must be 1 or 2 hops."));
        return false;
    }

    char normalizedSsid[WIFI_SSID_CAPACITY] = {};
    char normalizedPassword[WIFI_PASSWORD_CAPACITY] = {};
    char normalizedServer[APRS_IS_SERVER_CAPACITY] = {};
    char normalizedFilter[APRS_IS_FILTER_CAPACITY] = {};
    if (!copyPrintable(wifiSsid != nullptr ? wifiSsid : "", normalizedSsid, sizeof(normalizedSsid), true)) {
        setError(errorText, errorTextCapacity, App::Localization::text("WiFi SSID je prilis dlouhe nebo obsahuje nepovolene znaky.", "Wi-Fi SSID is too long or contains invalid characters."));
        return false;
    }
    if (!copyPrintable(wifiPassword != nullptr ? wifiPassword : "", normalizedPassword, sizeof(normalizedPassword), true)) {
        setError(errorText, errorTextCapacity, App::Localization::text("WiFi heslo je prilis dlouhe nebo obsahuje nepovolene znaky.", "Wi-Fi password is too long or contains invalid characters."));
        return false;
    }
    if (!normalizeServer(aprsIsServer, normalizedServer, sizeof(normalizedServer))) {
        setError(errorText, errorTextCapacity, App::Localization::text("Neplatny APRS-IS server.", "Invalid APRS-IS server."));
        return false;
    }
    if (!normalizeFilter(aprsIsFilter, normalizedFilter, sizeof(normalizedFilter))) {
        setError(errorText, errorTextCapacity, App::Localization::text("Neplatny APRS-IS filter.", "Invalid APRS-IS filter."));
        return false;
    }
    if (aprsIsPort == 0) {
        setError(errorText, errorTextCapacity, App::Localization::text("APRS-IS port musi byt 1 az 65535.", "APRS-IS port must be between 1 and 65535."));
        return false;
    }
    if (aprsIsPasscode < -1 || aprsIsPasscode > 32767) {
        setError(errorText, errorTextCapacity, App::Localization::text("APRS-IS passcode musi byt -1 nebo 0 az 32767.", "APRS-IS passcode must be -1 or between 0 and 32767."));
        return false;
    }
    if (igateEnabled && !validAprsIsRfCallsign(view_.callsign)) {
        setError(
            errorText,
            errorTextCapacity,
            App::Localization::text("Pro iGate pouzijte platny RF CALL (3-6 znaku, volitelne SSID 1-15; ne -0).", "Use a valid RF callsign for iGate (3-6 characters, optional SSID 1-15; not -0)."));
        return false;
    }
    if (igateEnabled && normalizedSsid[0] == '\0') {
        setError(errorText, errorTextCapacity, App::Localization::text("Pro iGate zadejte WiFi SSID.", "Enter a Wi-Fi SSID for iGate."));
        return false;
    }
    if (igateEnabled && aprsIsPasscode < 0) {
        setError(errorText, errorTextCapacity, App::Localization::text("iGate vyzaduje overeny APRS-IS passcode.", "iGate requires a verified APRS-IS passcode."));
        return false;
    }

    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        setError(errorText, errorTextCapacity, App::Localization::text("Nelze otevrit NVS pro zapis.", "Cannot open NVS for writing."));
        return false;
    }

    const bool digiEnabledSaved =
        preferences.putBool(KEY_DIGI_ENABLED, digiEnabled) > 0;
    const bool digiModeSaved = preferences.putUChar(
        KEY_DIGI_MODE,
        static_cast<std::uint8_t>(digiMode)) > 0;
    const bool digiMaxSaved = preferences.putUChar(KEY_DIGI_MAX, maxWideHops) > 0;
    const bool igateEnabledSaved =
        preferences.putBool(KEY_IGATE_ENABLED, igateEnabled) > 0;
    const bool ssidSaved = saveOptionalString(preferences, KEY_WIFI_SSID, normalizedSsid);
    const bool passwordSaved =
        saveOptionalString(preferences, KEY_WIFI_PASSWORD, normalizedPassword);
    const bool serverSaved =
        preferences.putString(KEY_APRS_IS_SERVER, normalizedServer) > 0;
    const bool portSaved = preferences.putUInt(KEY_APRS_IS_PORT, aprsIsPort) > 0;
    const bool passcodeSaved =
        preferences.putInt(KEY_APRS_IS_PASSCODE, aprsIsPasscode) > 0;
    const bool filterSaved =
        saveOptionalString(preferences, KEY_APRS_IS_FILTER, normalizedFilter);
    preferences.end();

    const bool saved = digiEnabledSaved && digiModeSaved && digiMaxSaved &&
        igateEnabledSaved && ssidSaved && passwordSaved && serverSaved &&
        portSaved && passcodeSaved && filterSaved;
    if (!saved) {
        setError(errorText, errorTextCapacity, App::Localization::text("Ulozeni DIGI/iGate do NVS se nepodarilo.", "Saving DIGI/iGate settings to NVS failed."));
        return false;
    }

    view_.digiEnabled = digiEnabled;
    view_.digiMode = digiMode;
    view_.digiMaxWideHops = maxWideHops;
    view_.igateEnabled = igateEnabled;
    std::snprintf(view_.wifiSsid, sizeof(view_.wifiSsid), "%s", normalizedSsid);
    std::snprintf(view_.wifiPassword, sizeof(view_.wifiPassword), "%s", normalizedPassword);
    std::snprintf(view_.aprsIsServer, sizeof(view_.aprsIsServer), "%s", normalizedServer);
    view_.aprsIsPort = aprsIsPort;
    view_.aprsIsPasscode = aprsIsPasscode;
    std::snprintf(view_.aprsIsFilter, sizeof(view_.aprsIsFilter), "%s", normalizedFilter);
    view_.persistentStorageReady = true;
    ++view_.revision;
    setError(errorText, errorTextCapacity, "");
    LOG_I(
        "SETTINGS",
        "DIGI/iGate saved digi=%u mode=%u max=%u igate=%u server=%s:%u",
        digiEnabled ? 1U : 0U,
        static_cast<unsigned>(digiMode),
        static_cast<unsigned>(maxWideHops),
        igateEnabled ? 1U : 0U,
        view_.aprsIsServer,
        static_cast<unsigned>(view_.aprsIsPort));
    return true;
}

const SettingsService::ViewState& SettingsService::viewState() const {
    return view_;
}

bool SettingsService::normalizeCallsign(
    const char* input,
    char* output,
    std::size_t outputCapacity) {

    if (input == nullptr || output == nullptr || outputCapacity < 4) {
        return false;
    }

    char temporary[CALLSIGN_CAPACITY] = {};
    std::size_t length = 0;
    for (const char* cursor = input; *cursor != '\0'; ++cursor) {
        char ch = *cursor;
        if (ch == ' ' || ch == '\t') {
            continue;
        }
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
        if (!((ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '-')) {
            return false;
        }
        if (length + 1 >= sizeof(temporary)) {
            return false;
        }
        temporary[length++] = ch;
    }
    temporary[length] = '\0';

    const char* dash = std::strchr(temporary, '-');
    const std::size_t baseLength = dash == nullptr
        ? length
        : static_cast<std::size_t>(dash - temporary);
    if (baseLength < 1 || baseLength > 6) {
        return false;
    }
    for (std::size_t index = 0; index < baseLength; ++index) {
        const char ch = temporary[index];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))) {
            return false;
        }
    }

    if (dash != nullptr) {
        if (std::strchr(dash + 1, '-') != nullptr || dash[1] == '\0') {
            return false;
        }
        int ssid = 0;
        std::size_t digits = 0;
        for (const char* cursor = dash + 1; *cursor != '\0'; ++cursor) {
            if (*cursor < '0' || *cursor > '9' || digits >= 2) {
                return false;
            }
            ssid = ssid * 10 + (*cursor - '0');
            ++digits;
        }
        if (ssid < 0 || ssid > 15) {
            return false;
        }
    }

    if (length + 1 > outputCapacity) {
        return false;
    }
    std::memcpy(output, temporary, length + 1);
    return true;
}

void SettingsService::setError(
    char* output,
    std::size_t outputCapacity,
    const char* text) {

    if (output == nullptr || outputCapacity == 0) {
        return;
    }
    std::snprintf(output, outputCapacity, "%s", text != nullptr ? text : "");
}

}  // namespace Services
