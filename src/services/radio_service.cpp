#include "services/radio_service.h"

#include <Arduino.h>
#include <aprs_codec.h>
#include <cstdio>
#include <cstring>

#include "app_config.h"
#include "app_log.h"
#include "lora_profile.h"

namespace Services {

bool RadioService::begin() {
    view_ = ViewState{};
    stationStore_.clear();
    weatherStore_.clear();
    messageStore_.clear();
    digiIgate_.begin();
    const bool ready = driver_.begin();
    refreshDriverStatus();
    return ready;
}

void RadioService::update(
    std::uint32_t now,
    const SettingsService::ViewState& settings) {

    // Called on every main-loop pass. RX, station/weather history, APRS
    // messaging, DIGI and iGate therefore continue regardless of screen.
    digiIgate_.update(now, settings);
    driver_.update(now);

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

        // Binary-safe protocol input is required for Mic-E.
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
                stationStore_.ingest(parsed, packet.rssiDbm, packet.snrDb, now);
                weatherStore_.ingest(parsed, packet.rssiDbm, packet.snrDb, now);
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
            LOG_D("APRS", "RX payload decode failed");
        }

        LOG_I("APRS", "RX: %s", view_.lastPacketText);
    }

    messageStore_.update(now);
    refreshDriverStatus();
    serviceMessageTransmit(now, settings.callsign);
    refreshDriverStatus();
    serviceDigiTransmit(now);
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
    return sendTnc2(frame, now);
}

bool RadioService::sendTnc2(const char* frame, std::uint32_t now) {
    if (frame == nullptr) {
        return false;
    }
    return sendTnc2Bytes(
        reinterpret_cast<const std::uint8_t*>(frame),
        std::strlen(frame),
        now);
}

bool RadioService::sendTnc2Bytes(
    const std::uint8_t* frame,
    std::size_t frameLength,
    std::uint32_t now) {

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
    const bool started = driver_.startTransmit(
        packet,
        headerLength + frameLength,
        now);
    if (started) {
        char printable[220] = {};
        const std::size_t copyLength = frameLength < sizeof(printable) - 1
            ? frameLength
            : sizeof(printable) - 1;
        for (std::size_t index = 0; index < copyLength; ++index) {
            const std::uint8_t value = frame[index];
            printable[index] = value >= 32 && value <= 126
                ? static_cast<char>(value)
                : '.';
        }
        LOG_I("APRS", "TX queued: %s", printable);
    }
    refreshDriverStatus();
    return started;
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
}

void RadioService::serviceMessageTransmit(
    std::uint32_t now,
    const char* ownCallsign) {

    if (!view_.initialized || view_.transmitting || ownCallsign == nullptr ||
        ownCallsign[0] == '\0') {
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

    if (sendTnc2(frame, now)) {
        messageStore_.markTransmissionStarted(token, now);
    }
}

void RadioService::serviceDigiTransmit(std::uint32_t now) {
    if (!view_.initialized || view_.transmitting) {
        return;
    }

    std::uint8_t frame[LoRaProfile::MAX_PACKET_LENGTH] = {};
    std::size_t frameLength = 0;
    if (!digiIgate_.takeDigiFrame(
            now,
            frame,
            sizeof(frame),
            frameLength)) {
        return;
    }
    const bool started = sendTnc2Bytes(frame, frameLength, now);
    digiIgate_.markDigiTransmitResult(started);
}

}  // namespace Services
