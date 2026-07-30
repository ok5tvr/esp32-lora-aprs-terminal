#pragma once

#include <cstddef>
#include <cstdint>

#include "drivers/sx1278_driver.h"
#include "services/digi_igate_service.h"
#include "services/message_store.h"
#include "services/station_store.h"
#include "services/tx_queue.h"
#include "services/weather_store.h"

namespace Services {

class RadioService {
public:
    struct ViewState {
        bool initialized = false;
        bool receiving = false;
        bool transmitting = false;
        bool lastPacketHadOeHeader = false;
        std::int16_t lastError = 0;
        std::uint32_t receivedPackets = 0;
        std::uint32_t validAprsPackets = 0;
        std::uint32_t decodeErrors = 0;
        std::uint32_t transmittedPackets = 0;
        std::uint32_t receiveErrors = 0;
        std::uint32_t transmitTimeouts = 0;
        std::uint8_t consecutiveReceiveErrors = 0;
        std::uint8_t txQueueDepth = 0;
        std::uint8_t txQueueMaximumDepth = 0;
        std::uint32_t txQueueEnqueued = 0;
        std::uint32_t txQueueReplaced = 0;
        std::uint32_t txQueueDrops = 0;
        std::uint32_t recoveryAttempts = 0;
        std::uint32_t successfulRecoveries = 0;
        std::uint32_t recoveryFailures = 0;
        std::uint32_t lastRxAtMs = 0;
        std::uint32_t lastTxAtMs = 0;
        float lastRssiDbm = 0.0F;
        float lastSnrDb = 0.0F;
        float lastFrequencyErrorHz = 0.0F;
        char lastTxSource[16] = "--";
        char lastRecoveryText[64] = "Zatim nebyla potreba";
        char lastPacketText[256] = "Cekam na prvni paket...";
    };

    bool begin();
    void update(
        std::uint32_t now,
        const SettingsService::ViewState& settings);
    bool sendTestPacket(const char* callsign, std::uint32_t now);
    bool queueTrackerPacket(const char* frame, bool manual, std::uint32_t now);
    bool sendTnc2(const char* frame, std::uint32_t now);
    bool queueMessage(
        const char* recipient,
        const char* text,
        std::uint32_t now,
        char* errorText,
        std::size_t errorTextCapacity);
    const ViewState& viewState() const;
    const StationStore::ViewState& stationViewState() const;
    const WeatherStore::ViewState& weatherViewState() const;
    const MessageStore::ViewState& messageViewState() const;
    const DigiIgateService::ViewState& digiIgateViewState() const;

private:
    void refreshDriverStatus();
    void observeDriverEvents(std::uint32_t now);
    void refreshQueueStatus();
    void serviceMessageQueue(std::uint32_t now, const char* ownCallsign);
    void serviceDigiQueue(std::uint32_t now);
    void serviceTxQueue(std::uint32_t now);
    void serviceRecovery(std::uint32_t now);
    bool enqueueTnc2Bytes(
        const std::uint8_t* frame,
        std::size_t frameLength,
        TxQueue::Source source,
        TxQueue::Priority priority,
        std::uint32_t now,
        bool replaceSameSource = false,
        const char* tagPeer = nullptr,
        const char* tagId = nullptr);
    void onTransmissionStarted(const TxQueue::Item& item, std::uint32_t now);

    Drivers::Sx1278Driver driver_;
    StationStore stationStore_;
    WeatherStore weatherStore_;
    MessageStore messageStore_;
    DigiIgateService digiIgate_;
    TxQueue txQueue_;
    ViewState view_;
    std::uint32_t lastTxStartedAt_ = 0;
    std::uint32_t lastTxCompletedAt_ = 0;
    std::uint32_t observedCompletedTransmissions_ = 0;
    bool wasTransmitting_ = false;
    std::uint32_t lastRecoveryAttemptAt_ = 0;
    std::uint32_t observedTransmitTimeouts_ = 0;
};

}  // namespace Services
