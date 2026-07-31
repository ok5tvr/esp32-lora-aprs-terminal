#include "services/radio_service.h"

#include <Arduino.h>
#include <aprs_codec.h>
#include <cstdio>
#include <cstring>

#include "app_config.h"
#include "app/localization.h"
#include "app_log.h"
#include "lora_profile.h"

namespace Services {
namespace {

bool timeReached(std::uint32_t now, std::uint32_t target) {
    return static_cast<std::int32_t>(now - target) >= 0;
}

LoRaProfile::Config configFromSettings(
    const SettingsService::ViewState& settings) {

    LoRaProfile::Config config;
    config.frequencyMHz = settings.loraFrequencyMHz;
    config.bandwidthKHz = settings.loraBandwidthKHz;
    config.spreadingFactor = settings.loraSpreadingFactor;
    config.codingRate = settings.loraCodingRate;
    config.outputPowerDbm = settings.loraOutputPowerDbm;
    return LoRaProfile::isValidConfig(config)
        ? config
        : LoRaProfile::czeAprsConfig();
}

}  // namespace

bool RadioService::begin(const SettingsService::ViewState& settings) {
    view_ = ViewState{};
    localizationRevision_ = App::Localization::revision();
    noticeKind_ = NoticeKind::NotNeeded;
    recoveryReason_ = RecoveryReason::None;
    noticeError_ = 0;
    refreshLocalizedNotice();
    std::snprintf(
        view_.lastPacketText,
        sizeof(view_.lastPacketText),
        "%s",
        App::Localization::text("Cekam na prvni paket...", "Waiting for the first packet..."));
    stationStore_.clear();
    weatherStore_.clear();
    messageStore_.clear();
    digiIgate_.begin();
    txQueue_.clear();
    lastTxStartedAt_ = 0;
    lastTxCompletedAt_ = 0;
    observedCompletedTransmissions_ = 0;
    wasTransmitting_ = false;
    lastRecoveryAttemptAt_ = 0;
    observedTransmitTimeouts_ = 0;
    const std::uint32_t now = millis();
    nextNoiseMeasurementAt_ = now + AppConfig::RADIO_NOISE_INITIAL_DELAY_MS;
    lastNoiseBurstSampleAt_ = 0;
    noiseBurstAccumulator_ = 0.0F;
    noiseBurstPeakDbm_ = -170.0F;
    noiseBurstSamples_ = 0;
    noiseBurstActive_ = false;
    view_.noiseNextMeasurementAtMs = nextNoiseMeasurementAt_;
    desiredConfig_ = configFromSettings(settings);
    desiredPreset_ = settings.loraPreset;
    configurationPending_ = false;
    const bool ready = driver_.begin(desiredConfig_);
    updateConfigurationView();
    refreshDriverStatus();
    refreshQueueStatus();
    return ready;
}

void RadioService::requestConfiguration(
    const SettingsService::ViewState& settings) {

    const LoRaProfile::Config requested = configFromSettings(settings);
    desiredPreset_ = settings.loraPreset;
    if (LoRaProfile::sameConfig(requested, driver_.config())) {
        // A second save can cancel a not-yet-applied change by selecting the
        // currently active RF parameters again.
        desiredConfig_ = requested;
        const bool cancelledPendingChange = configurationPending_;
        configurationPending_ = false;
        updateConfigurationView();
        if (cancelledPendingChange && noticeKind_ == NoticeKind::ConfigurationWaiting) {
            noticeKind_ = NoticeKind::NotNeeded;
            refreshLocalizedNotice();
        }
        return;
    }

    desiredConfig_ = requested;
    configurationPending_ = true;
    view_.loraConfigurationPending = true;
    noticeKind_ = NoticeKind::ConfigurationWaiting;
    refreshLocalizedNotice();
    LOG_I(
        "RADIO",
        "Configuration requested %.3f MHz BW %.1f SF%u CR4/%u P%d",
        static_cast<double>(desiredConfig_.frequencyMHz),
        static_cast<double>(desiredConfig_.bandwidthKHz),
        static_cast<unsigned>(desiredConfig_.spreadingFactor),
        static_cast<unsigned>(desiredConfig_.codingRate),
        static_cast<int>(desiredConfig_.outputPowerDbm));
}

void RadioService::update(
    std::uint32_t now,
    const SettingsService::ViewState& settings) {

    if (localizationRevision_ != App::Localization::revision()) {
        localizationRevision_ = App::Localization::revision();
        refreshLocalizedNotice();
        if (view_.receivedPackets == 0U) {
            std::snprintf(
                view_.lastPacketText,
                sizeof(view_.lastPacketText),
                "%s",
                App::Localization::text(
                    "Cekam na prvni paket...",
                    "Waiting for the first packet..."));
        }
    }

    // Keep network, RF reception and protocol parsing ahead of queued TX work.
    digiIgate_.update(now, settings);
    serviceRecovery(now);
    driver_.update(now);
    observeDriverEvents(now);

    Drivers::Sx1278Driver::Packet packet;
    if (driver_.takePacket(packet)) {
        Aprs::decodeText(
            packet.data,
            packet.length,
            view_.lastPacketText,
            sizeof(view_.lastPacketText),
            view_.lastPacketHadOeHeader);
        view_.lastRssiDbm = packet.rssiDbm;
        view_.lastSnrDb = packet.snrDb;
        view_.lastFrequencyErrorHz = packet.frequencyErrorHz;
        view_.lastRxAtMs = now;

        std::uint8_t rawTnc2[LoRaProfile::MAX_PACKET_LENGTH + 1] = {};
        std::size_t rawTnc2Length = 0;
        bool rawHadOeHeader = false;
        if (Aprs::decodeRaw(
                packet.data,
                packet.length,
                rawTnc2,
                sizeof(rawTnc2),
                rawTnc2Length,
                rawHadOeHeader)) {
            Aprs::ParsedFrame parsed;
            if (Aprs::parseTnc2(rawTnc2, rawTnc2Length, parsed)) {
                ++view_.validAprsPackets;
                stationStore_.ingest(
                    parsed,
                    packet.rssiDbm,
                    packet.snrDb,
                    now,
                    view_.lastPacketText);
                weatherStore_.ingest(parsed, packet.rssiDbm, packet.snrDb, now, view_.lastPacketText);
                if (parsed.hasPosition) {
                    LOG_I(
                        "APRS",
                        "Entity %s via %s %c%c %.5f %.5f",
                        parsed.entityName,
                        parsed.source,
                        parsed.symbolTable,
                        parsed.symbolCode,
                        parsed.latitude,
                        parsed.longitude);
                }
            } else {
                ++view_.decodeErrors;
            }

            digiIgate_.ingestRfFrame(rawTnc2, rawTnc2Length, now);

            Aprs::ParsedMessage parsedMessage;
            if (Aprs::parseMessageTnc2(rawTnc2, rawTnc2Length, parsedMessage)) {
                messageStore_.ingest(parsedMessage, settings.callsign, now);
                LOG_I(
                    "APRS-MSG",
                    "RX %s -> %s id=%s text=%s",
                    parsedMessage.source,
                    parsedMessage.addressee,
                    parsedMessage.hasMessageId ? parsedMessage.messageId : "-",
                    parsedMessage.text);
            }
        } else {
            ++view_.decodeErrors;
            LOG_D("APRS", "RX payload decode failed");
        }

        LOG_I("APRS", "RX: %s", view_.lastPacketText);
    }

    servicePendingConfiguration(now);
    messageStore_.update(now);
    refreshDriverStatus();
    serviceNoiseMonitor(now);
    serviceMessageQueue(now, settings.callsign);
    serviceDigiQueue(now);
    serviceTxQueue(now);
    refreshDriverStatus();
    serviceRecovery(now);
    refreshDriverStatus();
    refreshQueueStatus();
}

void RadioService::updateConfigurationView() {
    const LoRaProfile::Config& active = driver_.config();
    view_.loraPreset = desiredPreset_;
    view_.loraFrequencyMHz = active.frequencyMHz;
    view_.loraBandwidthKHz = active.bandwidthKHz;
    view_.loraSpreadingFactor = active.spreadingFactor;
    view_.loraCodingRate = active.codingRate;
    view_.loraOutputPowerDbm = active.outputPowerDbm;
    view_.loraConfigurationPending = configurationPending_;
}

void RadioService::servicePendingConfiguration(std::uint32_t now) {
    if (!configurationPending_) {
        return;
    }

    const Drivers::Sx1278Driver::Status& status = driver_.status();
    if (status.mode == Drivers::Sx1278Driver::Mode::Transmitting ||
        txQueue_.stats().depth != 0U) {
        return;
    }

    cancelNoiseBurst(now);
    const bool ready = driver_.reconfigure(desiredConfig_);
    configurationPending_ = false;
    updateConfigurationView();
    refreshDriverStatus();
    nextNoiseMeasurementAt_ = now + AppConfig::RADIO_NOISE_INITIAL_DELAY_MS;
    view_.noiseNextMeasurementAtMs = nextNoiseMeasurementAt_;

    if (ready) {
        noticeKind_ = NoticeKind::ConfigurationApplied;
        noticeError_ = 0;
        refreshLocalizedNotice();
        LOG_I("RADIO", "LoRa configuration applied");
    } else {
        noticeKind_ = NoticeKind::ConfigurationFailed;
        noticeError_ = driver_.status().lastError;
        refreshLocalizedNotice();
        LOG_E(
            "RADIO",
            "LoRa configuration failed: %d",
            static_cast<int>(driver_.status().lastError));
    }
}

void RadioService::serviceNoiseMonitor(std::uint32_t now) {
    view_.noiseNextMeasurementAtMs = nextNoiseMeasurementAt_;
    view_.noiseMeasurementActive = noiseBurstActive_;
    view_.noiseBurstProgress = noiseBurstSamples_;

    const bool radioIdle = view_.initialized && view_.receiving && !view_.transmitting &&
        txQueue_.stats().depth == 0U;

    if (!noiseBurstActive_) {
        if (!timeReached(now, nextNoiseMeasurementAt_)) {
            return;
        }
        if (!radioIdle) {
            nextNoiseMeasurementAt_ = now + AppConfig::RADIO_NOISE_RETRY_DELAY_MS;
            view_.noiseNextMeasurementAtMs = nextNoiseMeasurementAt_;
            return;
        }

        noiseBurstActive_ = true;
        noiseBurstAccumulator_ = 0.0F;
        noiseBurstPeakDbm_ = -170.0F;
        noiseBurstSamples_ = 0;
        lastNoiseBurstSampleAt_ = now - AppConfig::RADIO_NOISE_BURST_SPACING_MS;
        view_.noiseMeasurementActive = true;
        view_.noiseBurstProgress = 0;
    }

    if (!radioIdle) {
        cancelNoiseBurst(now);
        return;
    }

    if (!timeReached(now, lastNoiseBurstSampleAt_ + AppConfig::RADIO_NOISE_BURST_SPACING_MS)) {
        return;
    }

    float sampleDbm = 0.0F;
    if (!driver_.readCurrentRssi(sampleDbm)) {
        cancelNoiseBurst(now);
        return;
    }

    noiseBurstAccumulator_ += sampleDbm;
    if (noiseBurstSamples_ == 0U || sampleDbm > noiseBurstPeakDbm_) {
        noiseBurstPeakDbm_ = sampleDbm;
    }
    ++noiseBurstSamples_;
    lastNoiseBurstSampleAt_ = now;
    view_.noiseBurstProgress = noiseBurstSamples_;

    if (noiseBurstSamples_ < AppConfig::RADIO_NOISE_BURST_SAMPLES) {
        return;
    }

    const float averageDbm = noiseBurstAccumulator_ /
        static_cast<float>(noiseBurstSamples_);
    appendNoiseMeasurement(averageDbm, noiseBurstPeakDbm_, now);

    noiseBurstActive_ = false;
    noiseBurstSamples_ = 0;
    noiseBurstAccumulator_ = 0.0F;
    noiseBurstPeakDbm_ = -170.0F;
    nextNoiseMeasurementAt_ = now + AppConfig::RADIO_NOISE_SAMPLE_INTERVAL_MS;
    view_.noiseMeasurementActive = false;
    view_.noiseBurstProgress = 0;
    view_.noiseNextMeasurementAtMs = nextNoiseMeasurementAt_;
}

void RadioService::appendNoiseMeasurement(
    float averageDbm,
    float peakDbm,
    std::uint32_t now) {

    const std::size_t capacity = AppConfig::RADIO_NOISE_HISTORY_LENGTH;
    std::size_t count = view_.noiseHistoryCount;
    if (count < capacity) {
        view_.noiseHistoryDbm[count] = averageDbm;
        view_.noisePeakHistoryDbm[count] = peakDbm;
        ++count;
    } else {
        std::memmove(
            &view_.noiseHistoryDbm[0],
            &view_.noiseHistoryDbm[1],
            (capacity - 1U) * sizeof(view_.noiseHistoryDbm[0]));
        std::memmove(
            &view_.noisePeakHistoryDbm[0],
            &view_.noisePeakHistoryDbm[1],
            (capacity - 1U) * sizeof(view_.noisePeakHistoryDbm[0]));
        view_.noiseHistoryDbm[capacity - 1U] = averageDbm;
        view_.noisePeakHistoryDbm[capacity - 1U] = peakDbm;
        count = capacity;
    }

    view_.noiseHistoryCount = static_cast<std::uint8_t>(count);
    view_.noiseLatestAverageDbm = averageDbm;
    view_.noiseLatestPeakDbm = peakDbm;
    view_.noiseLastMeasurementAtMs = now;

    float minimum = view_.noiseHistoryDbm[0];
    float maximum = view_.noiseHistoryDbm[0];
    float total = 0.0F;
    for (std::size_t index = 0; index < count; ++index) {
        const float value = view_.noiseHistoryDbm[index];
        if (value < minimum) {
            minimum = value;
        }
        if (value > maximum) {
            maximum = value;
        }
        total += value;
    }
    view_.noiseHistoryMinDbm = minimum;
    view_.noiseHistoryMaxDbm = maximum;
    view_.noiseHistoryAverageDbm = total / static_cast<float>(count);
    ++view_.noiseHistoryRevision;

    LOG_I(
        "NOISE",
        "Channel RSSI avg %.1f dBm, peak %.1f dBm (%u/%u)",
        static_cast<double>(averageDbm),
        static_cast<double>(peakDbm),
        static_cast<unsigned>(count),
        static_cast<unsigned>(capacity));
}

void RadioService::cancelNoiseBurst(std::uint32_t now) {
    noiseBurstActive_ = false;
    noiseBurstSamples_ = 0;
    noiseBurstAccumulator_ = 0.0F;
    noiseBurstPeakDbm_ = -170.0F;
    nextNoiseMeasurementAt_ = now + AppConfig::RADIO_NOISE_RETRY_DELAY_MS;
    view_.noiseMeasurementActive = false;
    view_.noiseBurstProgress = 0;
    view_.noiseNextMeasurementAtMs = nextNoiseMeasurementAt_;
}

bool RadioService::sendTestPacket(const char* callsign, std::uint32_t now) {
    char frame[160];
    std::snprintf(
        frame,
        sizeof(frame),
        "%s>%s:>Waveshare RA-02 test v%s",
        callsign != nullptr && callsign[0] != '\0' ? callsign : AppConfig::DEFAULT_CALLSIGN,
        AppConfig::APRS_DESTINATION,
        AppConfig::FIRMWARE_VERSION);
    return enqueueTnc2Bytes(
        reinterpret_cast<const std::uint8_t*>(frame),
        std::strlen(frame),
        TxQueue::Source::Test,
        TxQueue::Priority::Test,
        now);
}

bool RadioService::queueTrackerPacket(
    const char* frame,
    bool manual,
    std::uint32_t now) {

    if (frame == nullptr) {
        return false;
    }
    return enqueueTnc2Bytes(
        reinterpret_cast<const std::uint8_t*>(frame),
        std::strlen(frame),
        manual ? TxQueue::Source::ManualBeacon : TxQueue::Source::Tracker,
        manual ? TxQueue::Priority::ManualBeacon : TxQueue::Priority::Tracker,
        now,
        !manual);
}

bool RadioService::sendTnc2(const char* frame, std::uint32_t now) {
    return queueTrackerPacket(frame, false, now);
}

bool RadioService::enqueueTnc2Bytes(
    const std::uint8_t* frame,
    std::size_t frameLength,
    TxQueue::Source source,
    TxQueue::Priority priority,
    std::uint32_t now,
    bool replaceSameSource,
    const char* tagPeer,
    const char* tagId) {

    if (frame == nullptr || frameLength == 0) {
        return false;
    }
    const std::size_t headerLength = AppConfig::ENABLE_OE_LORA_APRS_HEADER
        ? Aprs::OE_LORA_HEADER_SIZE
        : 0;
    if (headerLength + frameLength > LoRaProfile::MAX_PACKET_LENGTH) {
        LOG_E("APRS", "Frame too long: %u", static_cast<unsigned>(frameLength));
        return false;
    }

    std::uint8_t packet[LoRaProfile::MAX_PACKET_LENGTH] = {};
    if (headerLength > 0) {
        std::memcpy(packet, Aprs::OE_LORA_HEADER, headerLength);
    }
    std::memcpy(packet + headerLength, frame, frameLength);

    const bool queued = txQueue_.enqueue(
        packet,
        headerLength + frameLength,
        source,
        priority,
        now,
        replaceSameSource,
        tagPeer,
        tagId);
    if (queued) {
        char printable[180] = {};
        const std::size_t copyLength = frameLength < sizeof(printable) - 1
            ? frameLength
            : sizeof(printable) - 1;
        for (std::size_t index = 0; index < copyLength; ++index) {
            const std::uint8_t value = frame[index];
            printable[index] = value >= 32 && value <= 126
                ? static_cast<char>(value)
                : '.';
        }
        LOG_I("TXQ", "%s queued: %s", TxQueue::sourceName(source), printable);
    } else {
        LOG_E("TXQ", "%s queue full or invalid frame", TxQueue::sourceName(source));
    }
    refreshQueueStatus();
    return queued;
}

bool RadioService::queueMessage(
    const char* recipient,
    const char* text,
    std::uint32_t now,
    char* errorText,
    std::size_t errorTextCapacity) {

    return messageStore_.queueOutgoing(
        recipient,
        text,
        now,
        errorText,
        errorTextCapacity);
}

const RadioService::ViewState& RadioService::viewState() const {
    return view_;
}

const StationStore::ViewState& RadioService::stationViewState() const {
    return stationStore_.viewState();
}

const WeatherStore::ViewState& RadioService::weatherViewState() const {
    return weatherStore_.viewState();
}

const MessageStore::ViewState& RadioService::messageViewState() const {
    return messageStore_.viewState();
}

const DigiIgateService::ViewState& RadioService::digiIgateViewState() const {
    return digiIgate_.viewState();
}

void RadioService::refreshDriverStatus() {
    const Drivers::Sx1278Driver::Status& status = driver_.status();
    view_.initialized = status.initialized;
    view_.receiving = status.mode == Drivers::Sx1278Driver::Mode::Receiving;
    view_.transmitting = status.mode == Drivers::Sx1278Driver::Mode::Transmitting;
    view_.lastError = status.lastError;
    view_.receivedPackets = status.receivedPackets;
    view_.transmittedPackets = status.transmittedPackets;
    view_.receiveErrors = status.receiveErrors;
    view_.transmitTimeouts = status.transmitTimeouts;
    view_.consecutiveReceiveErrors = status.consecutiveReceiveErrors;
}


void RadioService::observeDriverEvents(std::uint32_t now) {
    const Drivers::Sx1278Driver::Status& status = driver_.status();
    const bool transmitting = status.mode == Drivers::Sx1278Driver::Mode::Transmitting;
    const std::uint32_t completed = status.transmittedPackets;
    if ((wasTransmitting_ && !transmitting) ||
        completed != observedCompletedTransmissions_) {
        lastTxCompletedAt_ = now;
    }
    observedCompletedTransmissions_ = completed;
    wasTransmitting_ = transmitting;
}

void RadioService::refreshQueueStatus() {
    const TxQueue::Stats& stats = txQueue_.stats();
    view_.txQueueDepth = stats.depth;
    view_.txQueueMaximumDepth = stats.maximumDepth;
    view_.txQueueEnqueued = stats.enqueued;
    view_.txQueueReplaced = stats.replaced;
    view_.txQueueDrops = stats.drops;
}

void RadioService::serviceMessageQueue(
    std::uint32_t now,
    const char* ownCallsign) {

    if (!view_.initialized || ownCallsign == nullptr || ownCallsign[0] == '\0' ||
        txQueue_.contains(TxQueue::Source::Acknowledgement)) {
        return;
    }

    char frame[192] = {};
    MessageStore::TxToken token;
    if (!messageStore_.prepareTransmission(
            ownCallsign,
            AppConfig::APRS_DESTINATION,
            now,
            frame,
            sizeof(frame),
            token)) {
        return;
    }

    const bool acknowledgement = token.kind == MessageStore::TxToken::Kind::Acknowledgement;
    if (!acknowledgement && txQueue_.contains(TxQueue::Source::Message)) {
        return;
    }
    enqueueTnc2Bytes(
        reinterpret_cast<const std::uint8_t*>(frame),
        std::strlen(frame),
        acknowledgement ? TxQueue::Source::Acknowledgement : TxQueue::Source::Message,
        acknowledgement ? TxQueue::Priority::Acknowledgement : TxQueue::Priority::Message,
        now,
        false,
        token.peer,
        token.messageId);
}

void RadioService::serviceDigiQueue(std::uint32_t now) {
    if (!view_.initialized ||
        txQueue_.contains(TxQueue::Source::Digipeater)) {
        return;
    }

    std::uint8_t frame[LoRaProfile::MAX_PACKET_LENGTH] = {};
    std::size_t frameLength = 0;
    if (!digiIgate_.takeDigiFrame(now, frame, sizeof(frame), frameLength)) {
        return;
    }
    const bool queued = enqueueTnc2Bytes(
        frame,
        frameLength,
        TxQueue::Source::Digipeater,
        TxQueue::Priority::Digipeater,
        now);
    if (!queued) {
        digiIgate_.markDigiTransmitResult(false);
    }
}

void RadioService::serviceTxQueue(std::uint32_t now) {
    if (!view_.initialized || view_.transmitting ||
        (lastTxCompletedAt_ != 0 && now - lastTxCompletedAt_ < AppConfig::RADIO_TX_MIN_GAP_MS)) {
        return;
    }

    TxQueue::Item candidate;
    std::size_t index = 0;
    if (!txQueue_.peek(candidate, index)) {
        return;
    }

    if (!driver_.startTransmit(candidate.data, candidate.length, now)) {
        refreshDriverStatus();
        return;
    }

    TxQueue::Item started;
    if (txQueue_.pop(index, started)) {
        lastTxStartedAt_ = now;
        wasTransmitting_ = true;
        view_.lastTxAtMs = now;
        std::snprintf(
            view_.lastTxSource,
            sizeof(view_.lastTxSource),
            "%s",
            TxQueue::sourceName(started.source));
        onTransmissionStarted(started, now);
    }
}

void RadioService::onTransmissionStarted(
    const TxQueue::Item& item,
    std::uint32_t now) {

    if (item.source == TxQueue::Source::Acknowledgement ||
        item.source == TxQueue::Source::Message) {
        MessageStore::TxToken token;
        token.kind = item.source == TxQueue::Source::Acknowledgement
            ? MessageStore::TxToken::Kind::Acknowledgement
            : MessageStore::TxToken::Kind::OutgoingMessage;
        std::snprintf(token.peer, sizeof(token.peer), "%s", item.tagPeer);
        std::snprintf(token.messageId, sizeof(token.messageId), "%s", item.tagId);
        messageStore_.markTransmissionStarted(token, now);
    } else if (item.source == TxQueue::Source::Digipeater) {
        digiIgate_.markDigiTransmitResult(true);
    }

    LOG_I("TXQ", "%s started, %u bytes", TxQueue::sourceName(item.source),
          static_cast<unsigned>(item.length));
}

const char* RadioService::recoveryReasonText() const {
    switch (recoveryReason_) {
        case RecoveryReason::RadioOffline:
            return App::Localization::text("radio offline", "radio offline");
        case RecoveryReason::ErrorState:
            return App::Localization::text("stav ERROR", "ERROR state");
        case RecoveryReason::RepeatedReceiveError:
            return App::Localization::text("opakovana RX chyba", "repeated RX error");
        case RecoveryReason::TransmitTimeout:
            return App::Localization::text("TX timeout", "TX timeout");
        default:
            return "--";
    }
}

void RadioService::refreshLocalizedNotice() {
    switch (noticeKind_) {
        case NoticeKind::ConfigurationWaiting:
            std::snprintf(
                view_.lastRecoveryText,
                sizeof(view_.lastRecoveryText),
                "%s",
                App::Localization::text(
                    "LoRa zmena ceka na volne radio",
                    "LoRa change is waiting for the radio to become idle"));
            break;
        case NoticeKind::ConfigurationApplied:
            std::snprintf(
                view_.lastRecoveryText,
                sizeof(view_.lastRecoveryText),
                "%s",
                App::Localization::text("LoRa profil aplikovan", "LoRa profile applied"));
            break;
        case NoticeKind::ConfigurationFailed:
            std::snprintf(
                view_.lastRecoveryText,
                sizeof(view_.lastRecoveryText),
                App::Localization::text("LoRa profil: chyba %d", "LoRa profile: error %d"),
                static_cast<int>(noticeError_));
            break;
        case NoticeKind::RecoverySucceeded:
            std::snprintf(
                view_.lastRecoveryText,
                sizeof(view_.lastRecoveryText),
                "OK: %s",
                recoveryReasonText());
            break;
        case NoticeKind::RecoveryFailed:
            std::snprintf(
                view_.lastRecoveryText,
                sizeof(view_.lastRecoveryText),
                App::Localization::text("SELHANI: %s", "FAILED: %s"),
                recoveryReasonText());
            break;
        case NoticeKind::NotNeeded:
        default:
            std::snprintf(
                view_.lastRecoveryText,
                sizeof(view_.lastRecoveryText),
                "%s",
                App::Localization::text("Zatim nebyla potreba", "Not needed yet"));
            break;
    }
}

void RadioService::serviceRecovery(std::uint32_t now) {
    const Drivers::Sx1278Driver::Status& status = driver_.status();
    const bool timeoutObserved = status.transmitTimeouts != observedTransmitTimeouts_;
    if (timeoutObserved) {
        observedTransmitTimeouts_ = status.transmitTimeouts;
    }

    RecoveryReason reason = RecoveryReason::None;
    if (!status.initialized) {
        reason = RecoveryReason::RadioOffline;
    } else if (status.mode == Drivers::Sx1278Driver::Mode::Error) {
        reason = RecoveryReason::ErrorState;
    } else if (status.consecutiveReceiveErrors >= AppConfig::RADIO_RECOVERY_RX_ERROR_THRESHOLD) {
        reason = RecoveryReason::RepeatedReceiveError;
    } else if (timeoutObserved) {
        reason = RecoveryReason::TransmitTimeout;
    }

    if (reason == RecoveryReason::None ||
        (lastRecoveryAttemptAt_ != 0 &&
         !timeReached(now, lastRecoveryAttemptAt_ + AppConfig::RADIO_RECOVERY_RETRY_MS))) {
        return;
    }

    recoveryReason_ = reason;
    lastRecoveryAttemptAt_ = now;
    ++view_.recoveryAttempts;
    LOG_E("RADIO", "Automatic recovery: %s", recoveryReasonText());
    const bool recovered = driver_.recover();
    if (recovered) {
        ++view_.successfulRecoveries;
        noticeKind_ = NoticeKind::RecoverySucceeded;
    } else {
        ++view_.recoveryFailures;
        noticeKind_ = NoticeKind::RecoveryFailed;
    }
    refreshLocalizedNotice();
}

}  // namespace Services
