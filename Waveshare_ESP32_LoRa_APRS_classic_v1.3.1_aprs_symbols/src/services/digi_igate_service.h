#pragma once

#include <WiFiClient.h>
#include <cstddef>
#include <cstdint>

#include "app/app_types.h"
#include "lora_profile.h"
#include "services/settings_service.h"

namespace Services {

class DigiIgateService {
public:
    static constexpr std::size_t DIGI_QUEUE_CAPACITY = 4;
    static constexpr std::size_t IGATE_QUEUE_CAPACITY = 8;
    // RF payloads can use the full LoRa packet. APRS-IS additionally needs
    // ",qAO,IGATECALL" before the data separator, so its queue must be larger.
    static constexpr std::size_t IGATE_LINE_CAPACITY =
        LoRaProfile::MAX_PACKET_LENGTH + 32;

    struct ViewState {
        bool digiEnabled = false;
        App::DigiMode digiMode = App::DigiMode::FillInWide1;
        std::uint8_t digiMaxWideHops = 2;
        bool igateEnabled = false;
        bool wifiConnected = false;
        bool aprsIsConnected = false;
        bool aprsIsVerified = false;
        bool loginRejected = false;
        std::int16_t wifiRssiDbm = 0;
        std::uint8_t digiQueueDepth = 0;
        std::uint8_t igateQueueDepth = 0;
        std::uint32_t digipeatedPackets = 0;
        std::uint32_t digiDuplicates = 0;
        std::uint32_t digiNotEligible = 0;
        std::uint32_t digiQueueDrops = 0;
        std::uint32_t gatedPackets = 0;
        std::uint32_t gateDuplicates = 0;
        std::uint32_t gateFiltered = 0;
        std::uint32_t gateQueueDrops = 0;
        std::uint32_t aprsIsReceivedLines = 0;
        std::uint32_t connectionAttempts = 0;
        char wifiAddress[20] = {};
        char statusText[112] = "DIGI/iGate vypnuto";
        char serverText[112] = "APRS-IS nepripojeno";
        std::uint32_t revision = 0;
    };

    bool begin();
    void update(
        std::uint32_t now,
        const SettingsService::ViewState& settings);
    void ingestRfFrame(
        const std::uint8_t* tnc2,
        std::size_t length,
        std::uint32_t now);
    bool takeDigiFrame(
        std::uint32_t now,
        std::uint8_t* output,
        std::size_t outputCapacity,
        std::size_t& outputLength);
    void markDigiTransmitResult(bool success);
    const ViewState& viewState() const;

private:
    struct Config {
        bool digiEnabled = false;
        App::DigiMode digiMode = App::DigiMode::FillInWide1;
        std::uint8_t digiMaxWideHops = 2;
        bool igateEnabled = false;
        char callsign[SettingsService::CALLSIGN_CAPACITY] = {};
        char wifiSsid[SettingsService::WIFI_SSID_CAPACITY] = {};
        char wifiPassword[SettingsService::WIFI_PASSWORD_CAPACITY] = {};
        char aprsIsServer[SettingsService::APRS_IS_SERVER_CAPACITY] = {};
        std::uint16_t aprsIsPort = 14580;
        std::int32_t aprsIsPasscode = -1;
        char aprsIsFilter[SettingsService::APRS_IS_FILTER_CAPACITY] = {};
    };

    struct QueuedFrame {
        std::uint8_t data[IGATE_LINE_CAPACITY] = {};
        std::size_t length = 0;
        std::uint32_t dueAt = 0;
    };

    struct DuplicateEntry {
        std::uint32_t hash = 0;
        std::uint32_t heardAt = 0;
    };

    bool applySettings(const SettingsService::ViewState& settings);
    void resetNetwork(bool turnWifiOff);
    void updateNetwork(std::uint32_t now);
    void ensureWifi(std::uint32_t now);
    void ensureAprsIs(std::uint32_t now);
    void readAprsIs(std::uint32_t now);
    void processServerLine(const char* line, std::uint32_t now);
    void serviceGateQueue(std::uint32_t now);
    bool sendLogin();
    bool enqueueDigi(
        const std::uint8_t* data,
        std::size_t length,
        std::uint32_t dueAt);
    bool enqueueGate(
        const std::uint8_t* data,
        std::size_t length,
        std::uint32_t heardAt);
    bool isDuplicate(
        DuplicateEntry* cache,
        std::size_t cacheSize,
        std::uint32_t hash,
        std::uint32_t now,
        bool remember);
    void clearDigiQueue();
    void clearGateQueue();
    void refreshView();

    Config config_;
    ViewState view_;
    WiFiClient aprsClient_;
    bool wifiStarted_ = false;
    std::uint32_t lastWifiAttemptAt_ = 0;
    std::uint32_t lastServerAttemptAt_ = 0;
    std::uint32_t lastServerDataAt_ = 0;
    char serverLine_[256] = {};
    std::size_t serverLineLength_ = 0;

    QueuedFrame digiQueue_[DIGI_QUEUE_CAPACITY] = {};
    std::size_t digiHead_ = 0;
    std::size_t digiCount_ = 0;
    QueuedFrame gateQueue_[IGATE_QUEUE_CAPACITY] = {};
    std::size_t gateHead_ = 0;
    std::size_t gateCount_ = 0;
    DuplicateEntry digiDuplicates_[16] = {};
    DuplicateEntry gateDuplicates_[16] = {};
};

}  // namespace Services
