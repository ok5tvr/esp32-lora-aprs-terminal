#include "services/digi_igate_service.h"

#include <Arduino.h>
#include <algorithm>
#include <WiFi.h>
#include <aprs_codec.h>
#include <cstdio>
#include <cstring>
#include <esp_system.h>

#include "app_config.h"
#include "app/localization.h"
#include "app_log.h"

namespace Services {
namespace {

bool timeReached(std::uint32_t now, std::uint32_t target) {
    return static_cast<std::int32_t>(now - target) >= 0;
}

void copyText(char* output, std::size_t capacity, const char* input) {
    if (output == nullptr || capacity == 0) {
        return;
    }
    std::snprintf(output, capacity, "%s", input != nullptr ? input : "");
}

const char* digiModeText(App::DigiMode mode) {
    switch (mode) {
        case App::DigiMode::FillInWide1: return "WIDE1-1";
        case App::DigiMode::TraceWide2: return "WIDE2-N";
        case App::DigiMode::FillInAndWide2: return "WIDE1+WIDE2";
        default: return "?";
    }
}

}  // namespace

bool DigiIgateService::begin() {
    config_ = Config{};
    view_ = ViewState{};
    copyText(
        view_.statusText,
        sizeof(view_.statusText),
        App::Localization::text("DIGI/iGate vypnuto", "DIGI/iGate disabled"));
    copyText(
        view_.serverText,
        sizeof(view_.serverText),
        App::Localization::text("APRS-IS nepripojeno", "APRS-IS disconnected"));
    clearDigiQueue();
    clearGateQueue();
    std::fill(std::begin(digiDuplicates_), std::end(digiDuplicates_), DuplicateEntry{});
    std::fill(std::begin(gateDuplicates_), std::end(gateDuplicates_), DuplicateEntry{});
    WiFi.persistent(false);
    return true;
}

void DigiIgateService::update(
    std::uint32_t now,
    const SettingsService::ViewState& settings) {

    const bool changed = applySettings(settings);
    if (changed) {
        // Do not transmit frames prepared with the previous CALL or path mode.
        clearDigiQueue();
        clearGateQueue();
        std::fill(std::begin(digiDuplicates_), std::end(digiDuplicates_), DuplicateEntry{});
        std::fill(std::begin(gateDuplicates_), std::end(gateDuplicates_), DuplicateEntry{});
        resetNetwork(!config_.igateEnabled);
    }

    if (config_.igateEnabled) {
        updateNetwork(now);
    }
    refreshView();
}

void DigiIgateService::ingestRfFrame(
    const std::uint8_t* tnc2,
    std::size_t length,
    std::uint32_t now) {

    if (tnc2 == nullptr || length == 0 || config_.callsign[0] == '\0') {
        return;
    }

    if (config_.digiEnabled) {
        const std::uint32_t hash = Aprs::tnc2PacketHash(tnc2, length);
        if (hash != 0 && isDuplicate(
                digiDuplicates_,
                sizeof(digiDuplicates_) / sizeof(digiDuplicates_[0]),
                hash,
                now,
                false)) {
            ++view_.digiDuplicates;
            ++view_.revision;
        } else {
            Aprs::DigipeaterOptions options;
            options.fillInWide1 =
                config_.digiMode == App::DigiMode::FillInWide1 ||
                config_.digiMode == App::DigiMode::FillInAndWide2;
            options.traceWide2 =
                config_.digiMode == App::DigiMode::TraceWide2 ||
                config_.digiMode == App::DigiMode::FillInAndWide2;
            options.maxWideHops = config_.digiMaxWideHops;

            std::uint8_t repeated[LoRaProfile::MAX_PACKET_LENGTH] = {};
            std::size_t repeatedLength = 0;
            const std::size_t oeHeaderLength = AppConfig::ENABLE_OE_LORA_APRS_HEADER
                ? Aprs::OE_LORA_HEADER_SIZE
                : 0U;
            const std::size_t rfFrameCapacity =
                sizeof(repeated) > oeHeaderLength
                    ? sizeof(repeated) - oeHeaderLength
                    : 0U;
            if (Aprs::buildDigipeatedTnc2(
                    tnc2,
                    length,
                    config_.callsign,
                    options,
                    repeated,
                    rfFrameCapacity,
                    repeatedLength)) {
                if (hash != 0) {
                    isDuplicate(
                        digiDuplicates_,
                        sizeof(digiDuplicates_) / sizeof(digiDuplicates_[0]),
                        hash,
                        now,
                        true);
                }
                const std::uint32_t spread =
                    AppConfig::DIGI_MAX_DELAY_MS - AppConfig::DIGI_MIN_DELAY_MS + 1U;
                const std::uint32_t dueAt = now + AppConfig::DIGI_MIN_DELAY_MS +
                    (spread > 0 ? esp_random() % spread : 0U);
                if (!enqueueDigi(repeated, repeatedLength, dueAt)) {
                    ++view_.digiQueueDrops;
                }
                ++view_.revision;
            } else {
                ++view_.digiNotEligible;
                ++view_.revision;
            }
        }
    }

    if (config_.igateEnabled) {
        std::uint8_t gated[IGATE_LINE_CAPACITY] = {};
        std::size_t gatedLength = 0;
        if (!Aprs::buildReceiveOnlyIgateTnc2(
                tnc2,
                length,
                config_.callsign,
                gated,
                sizeof(gated),
                gatedLength)) {
            ++view_.gateFiltered;
            ++view_.revision;
            return;
        }

        const std::uint32_t gateHash = Aprs::tnc2PacketHash(gated, gatedLength);
        if (gateHash != 0 && isDuplicate(
                gateDuplicates_,
                sizeof(gateDuplicates_) / sizeof(gateDuplicates_[0]),
                gateHash,
                now,
                false)) {
            ++view_.gateDuplicates;
            ++view_.revision;
            return;
        }
        if (gateHash != 0) {
            isDuplicate(
                gateDuplicates_,
                sizeof(gateDuplicates_) / sizeof(gateDuplicates_[0]),
                gateHash,
                now,
                true);
        }
        if (!enqueueGate(gated, gatedLength, now)) {
            ++view_.gateQueueDrops;
        }
        ++view_.revision;
    }
}

bool DigiIgateService::takeDigiFrame(
    std::uint32_t now,
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& outputLength) {

    outputLength = 0;
    if (digiCount_ == 0 || output == nullptr) {
        return false;
    }
    const QueuedFrame& frame = digiQueue_[digiHead_];
    if (!timeReached(now, frame.dueAt) || frame.length > outputCapacity) {
        return false;
    }
    std::memcpy(output, frame.data, frame.length);
    outputLength = frame.length;
    digiHead_ = (digiHead_ + 1) % DIGI_QUEUE_CAPACITY;
    --digiCount_;
    view_.digiQueueDepth = static_cast<std::uint8_t>(digiCount_);
    ++view_.revision;
    return true;
}

void DigiIgateService::markDigiTransmitResult(bool success) {
    if (success) {
        ++view_.digipeatedPackets;
    } else {
        ++view_.digiQueueDrops;
    }
    ++view_.revision;
}

const DigiIgateService::ViewState& DigiIgateService::viewState() const {
    return view_;
}

bool DigiIgateService::applySettings(const SettingsService::ViewState& settings) {
    Config next;
    next.digiEnabled = settings.digiEnabled;
    next.digiMode = settings.digiMode;
    next.digiMaxWideHops = settings.digiMaxWideHops;
    next.igateEnabled = settings.igateEnabled;
    next.aprsIsPort = settings.aprsIsPort;
    next.aprsIsPasscode = settings.aprsIsPasscode;
    copyText(next.callsign, sizeof(next.callsign), settings.callsign);
    copyText(next.wifiSsid, sizeof(next.wifiSsid), settings.wifiSsid);
    copyText(next.wifiPassword, sizeof(next.wifiPassword), settings.wifiPassword);
    copyText(next.aprsIsServer, sizeof(next.aprsIsServer), settings.aprsIsServer);
    copyText(next.aprsIsFilter, sizeof(next.aprsIsFilter), settings.aprsIsFilter);

    const bool unchanged =
        config_.digiEnabled == next.digiEnabled &&
        config_.digiMode == next.digiMode &&
        config_.digiMaxWideHops == next.digiMaxWideHops &&
        config_.igateEnabled == next.igateEnabled &&
        config_.aprsIsPort == next.aprsIsPort &&
        config_.aprsIsPasscode == next.aprsIsPasscode &&
        std::strcmp(config_.callsign, next.callsign) == 0 &&
        std::strcmp(config_.wifiSsid, next.wifiSsid) == 0 &&
        std::strcmp(config_.wifiPassword, next.wifiPassword) == 0 &&
        std::strcmp(config_.aprsIsServer, next.aprsIsServer) == 0 &&
        std::strcmp(config_.aprsIsFilter, next.aprsIsFilter) == 0;
    if (unchanged) {
        return false;
    }
    config_ = next;
    view_.digiEnabled = config_.digiEnabled;
    view_.digiMode = config_.digiMode;
    view_.digiMaxWideHops = config_.digiMaxWideHops;
    view_.igateEnabled = config_.igateEnabled;
    ++view_.revision;
    return true;
}

void DigiIgateService::resetNetwork(bool turnWifiOff) {
    if (aprsClient_.connected()) {
        aprsClient_.stop();
    }
    view_.aprsIsConnected = false;
    view_.aprsIsVerified = false;
    view_.loginRejected = false;
    serverLineLength_ = 0;
    lastWifiAttemptAt_ = 0;
    lastServerAttemptAt_ = 0;
    lastServerDataAt_ = 0;
    if (turnWifiOff) {
        clearGateQueue();
        if (wifiStarted_) {
            WiFi.disconnect(true, false);
            WiFi.mode(WIFI_OFF);
            wifiStarted_ = false;
        }
        view_.wifiConnected = false;
        view_.wifiAddress[0] = '\0';
    } else if (wifiStarted_) {
        WiFi.disconnect(false, false);
    }
}

void DigiIgateService::updateNetwork(std::uint32_t now) {
    ensureWifi(now);
    if (WiFi.status() != WL_CONNECTED) {
        view_.wifiConnected = false;
        view_.aprsIsConnected = false;
        view_.aprsIsVerified = false;
        return;
    }

    view_.wifiConnected = true;
    view_.wifiRssiDbm = static_cast<std::int16_t>(WiFi.RSSI());
    copyText(view_.wifiAddress, sizeof(view_.wifiAddress), WiFi.localIP().toString().c_str());
    ensureAprsIs(now);
    readAprsIs(now);
    serviceGateQueue(now);
}

void DigiIgateService::ensureWifi(std::uint32_t now) {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }
    if (wifiStarted_ && now - lastWifiAttemptAt_ < AppConfig::WIFI_RECONNECT_INTERVAL_MS) {
        return;
    }

    lastWifiAttemptAt_ = now;
    if (!wifiStarted_) {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        wifiStarted_ = true;
    } else {
        WiFi.disconnect(false, false);
    }
    WiFi.begin(config_.wifiSsid, config_.wifiPassword);
    LOG_I("IGATE", "Connecting WiFi SSID %s", config_.wifiSsid);
}

void DigiIgateService::ensureAprsIs(std::uint32_t now) {
    if (aprsClient_.connected()) {
        view_.aprsIsConnected = true;
        if (lastServerDataAt_ != 0 &&
            now - lastServerDataAt_ > AppConfig::APRS_IS_KEEPALIVE_TIMEOUT_MS) {
            LOG_E("IGATE", "%s", "APRS-IS keepalive timeout");
            aprsClient_.stop();
            view_.aprsIsConnected = false;
            view_.aprsIsVerified = false;
        }
        return;
    }

    view_.aprsIsConnected = false;
    view_.aprsIsVerified = false;
    view_.loginRejected = false;
    if (lastServerAttemptAt_ != 0 &&
        now - lastServerAttemptAt_ < AppConfig::APRS_IS_RECONNECT_INTERVAL_MS) {
        return;
    }

    lastServerAttemptAt_ = now;
    ++view_.connectionAttempts;
    ++view_.revision;
    LOG_I(
        "IGATE",
        "Connecting APRS-IS %s:%u",
        config_.aprsIsServer,
        static_cast<unsigned>(config_.aprsIsPort));
    if (!aprsClient_.connect(
            config_.aprsIsServer,
            config_.aprsIsPort,
            AppConfig::APRS_IS_CONNECT_TIMEOUT_MS)) {
        copyText(view_.serverText, sizeof(view_.serverText), App::Localization::text("APRS-IS spojeni selhalo", "APRS-IS connection failed"));
        return;
    }

    aprsClient_.setNoDelay(true);
    view_.aprsIsConnected = true;
    lastServerDataAt_ = now;
    if (!sendLogin()) {
        aprsClient_.stop();
        view_.aprsIsConnected = false;
    }
}

void DigiIgateService::readAprsIs(std::uint32_t now) {
    if (!aprsClient_.connected()) {
        return;
    }

    std::size_t processed = 0;
    while (aprsClient_.available() > 0 && processed < 768) {
        const int raw = aprsClient_.read();
        if (raw < 0) {
            break;
        }
        ++processed;
        lastServerDataAt_ = now;
        const char value = static_cast<char>(raw);
        if (value == '\n') {
            serverLine_[serverLineLength_] = '\0';
            processServerLine(serverLine_, now);
            serverLineLength_ = 0;
        } else if (value != '\r') {
            if (serverLineLength_ + 1 < sizeof(serverLine_)) {
                serverLine_[serverLineLength_++] = value;
            } else {
                serverLineLength_ = 0;
            }
        }
    }
}

void DigiIgateService::processServerLine(const char* line, std::uint32_t now) {
    (void)now;
    if (line == nullptr || line[0] == '\0') {
        return;
    }

    copyText(view_.serverText, sizeof(view_.serverText), line);
    if (line[0] != '#') {
        ++view_.aprsIsReceivedLines;
    }
    if (std::strstr(line, "logresp") != nullptr) {
        if (std::strstr(line, "unverified") != nullptr) {
            view_.aprsIsVerified = false;
            view_.loginRejected = true;
        } else if (std::strstr(line, "verified") != nullptr) {
            view_.aprsIsVerified = true;
            view_.loginRejected = false;
            LOG_I("IGATE", "%s", "APRS-IS login verified");
        }
    }
    ++view_.revision;
}

void DigiIgateService::serviceGateQueue(std::uint32_t now) {
    if (!aprsClient_.connected() || !view_.aprsIsVerified || gateCount_ == 0) {
        return;
    }

    // Do not flood APRS-IS with stale packets accumulated during an outage.
    while (gateCount_ > 0 &&
           now - gateQueue_[gateHead_].dueAt > AppConfig::APRS_IS_GATE_MAX_AGE_MS) {
        gateHead_ = (gateHead_ + 1) % IGATE_QUEUE_CAPACITY;
        --gateCount_;
        ++view_.gateQueueDrops;
        view_.igateQueueDepth = static_cast<std::uint8_t>(gateCount_);
        ++view_.revision;
    }
    if (gateCount_ == 0) {
        return;
    }

    const QueuedFrame& frame = gateQueue_[gateHead_];
    const std::size_t written = aprsClient_.write(frame.data, frame.length);
    const std::size_t ending = aprsClient_.write(
        reinterpret_cast<const std::uint8_t*>("\r\n"),
        2);
    if (written != frame.length || ending != 2) {
        LOG_E("IGATE", "%s", "APRS-IS write failed");
        aprsClient_.stop();
        view_.aprsIsConnected = false;
        view_.aprsIsVerified = false;
        return;
    }

    LOG_D("IGATE", "Gated %u bytes to APRS-IS", static_cast<unsigned>(frame.length));
    gateHead_ = (gateHead_ + 1) % IGATE_QUEUE_CAPACITY;
    --gateCount_;
    ++view_.gatedPackets;
    view_.igateQueueDepth = static_cast<std::uint8_t>(gateCount_);
    ++view_.revision;
}

bool DigiIgateService::sendLogin() {
    char line[256] = {};
    const int written = config_.aprsIsFilter[0] == '\0'
        ? std::snprintf(
              line,
              sizeof(line),
              "user %s pass %ld vers LoRaAPRS %s\r\n",
              config_.callsign,
              static_cast<long>(config_.aprsIsPasscode),
              AppConfig::FIRMWARE_VERSION)
        : std::snprintf(
              line,
              sizeof(line),
              "user %s pass %ld vers LoRaAPRS %s filter %s\r\n",
              config_.callsign,
              static_cast<long>(config_.aprsIsPasscode),
              AppConfig::FIRMWARE_VERSION,
              config_.aprsIsFilter);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(line)) {
        return false;
    }
    const std::size_t sent = aprsClient_.write(
        reinterpret_cast<const std::uint8_t*>(line),
        static_cast<std::size_t>(written));
    if (sent != static_cast<std::size_t>(written)) {
        return false;
    }
    copyText(view_.serverText, sizeof(view_.serverText), App::Localization::text("APRS-IS login odeslan", "APRS-IS login sent"));
    view_.aprsIsVerified = false;
    view_.loginRejected = false;
    return true;
}

bool DigiIgateService::enqueueDigi(
    const std::uint8_t* data,
    std::size_t length,
    std::uint32_t dueAt) {

    if (data == nullptr || length == 0 || length > LoRaProfile::MAX_PACKET_LENGTH ||
        digiCount_ >= DIGI_QUEUE_CAPACITY) {
        return false;
    }
    const std::size_t tail = (digiHead_ + digiCount_) % DIGI_QUEUE_CAPACITY;
    std::memcpy(digiQueue_[tail].data, data, length);
    digiQueue_[tail].length = length;
    digiQueue_[tail].dueAt = dueAt;
    ++digiCount_;
    view_.digiQueueDepth = static_cast<std::uint8_t>(digiCount_);
    return true;
}

bool DigiIgateService::enqueueGate(
    const std::uint8_t* data,
    std::size_t length,
    std::uint32_t heardAt) {
    if (data == nullptr || length == 0 || length > IGATE_LINE_CAPACITY ||
        length + 2 > 512 || gateCount_ >= IGATE_QUEUE_CAPACITY) {
        return false;
    }
    const std::size_t tail = (gateHead_ + gateCount_) % IGATE_QUEUE_CAPACITY;
    std::memcpy(gateQueue_[tail].data, data, length);
    gateQueue_[tail].length = length;
    gateQueue_[tail].dueAt = heardAt;
    ++gateCount_;
    view_.igateQueueDepth = static_cast<std::uint8_t>(gateCount_);
    return true;
}

bool DigiIgateService::isDuplicate(
    DuplicateEntry* cache,
    std::size_t cacheSize,
    std::uint32_t hash,
    std::uint32_t now,
    bool remember) {

    if (cache == nullptr || cacheSize == 0 || hash == 0) {
        return false;
    }
    std::size_t oldest = 0;
    std::uint32_t oldestAge = 0;
    for (std::size_t index = 0; index < cacheSize; ++index) {
        if (cache[index].hash == hash &&
            now - cache[index].heardAt <= AppConfig::DIGI_DUPLICATE_WINDOW_MS) {
            return true;
        }
        const std::uint32_t age = now - cache[index].heardAt;
        if (cache[index].hash == 0 || age >= oldestAge) {
            oldest = index;
            oldestAge = age;
        }
    }
    if (remember) {
        cache[oldest].hash = hash;
        cache[oldest].heardAt = now;
    }
    return false;
}

void DigiIgateService::clearDigiQueue() {
    digiHead_ = 0;
    digiCount_ = 0;
    view_.digiQueueDepth = 0;
}

void DigiIgateService::clearGateQueue() {
    gateHead_ = 0;
    gateCount_ = 0;
    view_.igateQueueDepth = 0;
}

void DigiIgateService::refreshView() {
    view_.digiEnabled = config_.digiEnabled;
    view_.digiMode = config_.digiMode;
    view_.digiMaxWideHops = config_.digiMaxWideHops;
    view_.igateEnabled = config_.igateEnabled;
    view_.wifiConnected = config_.igateEnabled && WiFi.status() == WL_CONNECTED;
    view_.aprsIsConnected = config_.igateEnabled && aprsClient_.connected();
    if (!view_.aprsIsConnected) {
        view_.aprsIsVerified = false;
        view_.loginRejected = false;
    }

    std::snprintf(
        view_.statusText,
        sizeof(view_.statusText),
        "DIGI %s %s max%u | TX %lu dup %lu q%u",
        config_.digiEnabled ? "ON" : "OFF",
        digiModeText(config_.digiMode),
        static_cast<unsigned>(config_.digiMaxWideHops),
        static_cast<unsigned long>(view_.digipeatedPackets),
        static_cast<unsigned long>(view_.digiDuplicates),
        static_cast<unsigned>(view_.digiQueueDepth));

    if (!config_.igateEnabled) {
        copyText(view_.serverText, sizeof(view_.serverText), App::Localization::text("iGate vypnuta (RF -> APRS-IS qAO)", "iGate disabled (RF -> APRS-IS qAO)"));
    } else if (!view_.wifiConnected) {
        copyText(view_.serverText, sizeof(view_.serverText), App::Localization::text("WiFi se pripojuje...", "Wi-Fi connecting..."));
    } else if (!view_.aprsIsConnected) {
        copyText(view_.serverText, sizeof(view_.serverText), App::Localization::text("APRS-IS se pripojuje...", "APRS-IS connecting..."));
    } else if (view_.loginRejected) {
        copyText(view_.serverText, sizeof(view_.serverText), App::Localization::text("APRS-IS login NEOVEREN", "APRS-IS login UNVERIFIED"));
    } else if (!view_.aprsIsVerified) {
        copyText(view_.serverText, sizeof(view_.serverText), App::Localization::text("APRS-IS ceka na overeni loginu", "APRS-IS waiting for login verification"));
    }
}

}  // namespace Services
