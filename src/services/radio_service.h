#pragma once

#include <cstddef>
#include <cstdint>

#include "app_config.h"
#include "drivers/sx1278_driver.h"
#include "services/digi_igate_service.h"
#include "services/message_store.h"
#include "services/settings_service.h"
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
        std::uint32_t lastCompletedTxSequence = 0;
        std::uint32_t lastFailedTxSequence = 0;
        std::uint32_t txCompletionRevision = 0;
        std::uint32_t txFailureRevision = 0;
        float lastRssiDbm = 0.0F;
        float lastSnrDb = 0.0F;
        float lastFrequencyErrorHz = 0.0F;
        App::LoRaPreset loraPreset = App::LoRaPreset::CzeAprs;
        float loraFrequencyMHz = LoRaProfile::FREQUENCY_MHZ;
        float loraBandwidthKHz = LoRaProfile::BANDWIDTH_KHZ;
        std::uint8_t loraSpreadingFactor = LoRaProfile::SPREADING_FACTOR;
        std::uint8_t loraCodingRate = LoRaProfile::CODING_RATE;
        std::int8_t loraOutputPowerDbm = LoRaProfile::OUTPUT_POWER_DBM;
        bool loraConfigurationPending = false;

        // Periodic background/channel RSSI monitoring. Each stored point is
        // an average of a short burst; peak history records the strongest
        // instantaneous value observed in the same burst.
        bool noiseMeasurementActive = false;
        std::uint8_t noiseBurstProgress = 0;
        std::uint8_t noiseHistoryCount = 0;
        std::uint32_t noiseHistoryRevision = 0;
        std::uint32_t noiseLastMeasurementAtMs = 0;
        std::uint32_t noiseNextMeasurementAtMs = 0;
        float noiseLatestAverageDbm = 0.0F;
        float noiseLatestPeakDbm = 0.0F;
        float noiseHistoryAverageDbm = 0.0F;
        float noiseHistoryMinDbm = 0.0F;
        float noiseHistoryMaxDbm = 0.0F;
        float noiseHistoryDbm[AppConfig::RADIO_NOISE_HISTORY_LENGTH] = {};
        float noisePeakHistoryDbm[AppConfig::RADIO_NOISE_HISTORY_LENGTH] = {};

        char lastTxSource[16] = "--";
        char lastRecoveryText[64] = "--";
        char lastPacketText[256] = "--";
    };

    bool begin(const SettingsService::ViewState& settings);
    void requestConfiguration(const SettingsService::ViewState& settings);
    void update(
        std::uint32_t now,
        const SettingsService::ViewState& settings);
    bool sendTestPacket(const char* callsign, std::uint32_t now);
    bool queueTrackerPacket(
        const char* frame,
        bool manual,
        std::uint32_t now,
        std::uint32_t* sequenceOut = nullptr);
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
    enum class NoticeKind : std::uint8_t {
        NotNeeded,
        ConfigurationWaiting,
        ConfigurationApplied,
        ConfigurationFailed,
        RecoverySucceeded,
        RecoveryFailed
    };

    enum class RecoveryReason : std::uint8_t {
        None,
        RadioOffline,
        ErrorState,
        RepeatedReceiveError,
        TransmitTimeout
    };

    void refreshLocalizedNotice();
    const char* recoveryReasonText() const;
    void refreshDriverStatus();
    void observeDriverEvents(std::uint32_t now);
    void refreshQueueStatus();
    void serviceMessageQueue(std::uint32_t now, const char* ownCallsign);
    void serviceDigiQueue(std::uint32_t now);
    void serviceTxQueue(std::uint32_t now);
    void serviceRecovery(std::uint32_t now);
    void servicePendingConfiguration(std::uint32_t now);
    void updateConfigurationView();
    void serviceNoiseMonitor(std::uint32_t now);
    void appendNoiseMeasurement(float averageDbm, float peakDbm, std::uint32_t now);
    void resetNoiseHistory(std::uint32_t now);
    void cancelNoiseBurst(std::uint32_t now);
    bool enqueueTnc2Bytes(
        const std::uint8_t* frame,
        std::size_t frameLength,
        TxQueue::Source source,
        TxQueue::Priority priority,
        std::uint32_t now,
        bool replaceSameSource = false,
        const char* tagPeer = nullptr,
        const char* tagId = nullptr,
        std::uint32_t* sequenceOut = nullptr);
    void onTransmissionStarted(const TxQueue::Item& item, std::uint32_t now);
    void onTransmissionCompleted(const TxQueue::Item& item, std::uint32_t now);
    void onTransmissionFailed(const TxQueue::Item& item, std::uint32_t now);

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
    TxQueue::Item activeTxItem_ = {};
    bool activeTxValid_ = false;
    std::uint32_t lastRecoveryAttemptAt_ = 0;
    std::uint32_t observedTransmitTimeouts_ = 0;
    std::uint32_t nextNoiseMeasurementAt_ = 0;
    std::uint32_t lastNoiseBurstSampleAt_ = 0;
    float noiseBurstAccumulator_ = 0.0F;
    float noiseBurstPeakDbm_ = -170.0F;
    std::uint8_t noiseBurstSamples_ = 0;
    bool noiseBurstActive_ = false;
    LoRaProfile::Config desiredConfig_ = LoRaProfile::czeAprsConfig();
    App::LoRaPreset desiredPreset_ = App::LoRaPreset::CzeAprs;
    bool configurationPending_ = false;
    std::uint32_t localizationRevision_ = 0;
    NoticeKind noticeKind_ = NoticeKind::NotNeeded;
    RecoveryReason recoveryReason_ = RecoveryReason::None;
    std::int16_t noticeError_ = 0;
};

}  // namespace Services
