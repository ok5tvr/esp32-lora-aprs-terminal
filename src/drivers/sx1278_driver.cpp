#include "drivers/sx1278_driver.h"

#include <Arduino.h>
#include <cstring>

#include "app_config.h"
#include "app_log.h"
#include "board_pins.h"

namespace Drivers {

Sx1278Driver* Sx1278Driver::activeInstance_ = nullptr;

Sx1278Driver::Sx1278Driver()
    : spi_(HSPI),
      spiSettings_(LoRaProfile::SPI_FREQUENCY_HZ, MSBFIRST, SPI_MODE0),
      module_(
          BoardPins::LORA_CS,
          BoardPins::LORA_DIO0,
          BoardPins::LORA_RESET,
          BoardPins::LORA_DIO1,
          spi_,
          spiSettings_),
      radio_(&module_) {
}

#if defined(ESP32)
IRAM_ATTR
#endif
void Sx1278Driver::onRadioInterrupt() {
    if (activeInstance_ != nullptr) {
        activeInstance_->interruptFlag_ = true;
    }
}

bool Sx1278Driver::begin() {
    status_ = Status{};
    interruptFlag_ = false;
    packetAvailable_ = false;
    transmitLength_ = 0;
    interruptAction_ = InterruptAction::None;
    activeInstance_ = this;

    pinMode(BoardPins::LORA_DIO0, INPUT_PULLDOWN);
    pinMode(BoardPins::LORA_CS, OUTPUT);
    digitalWrite(BoardPins::LORA_CS, HIGH);
    spi_.begin(
        BoardPins::LORA_SCLK,
        BoardPins::LORA_MISO,
        BoardPins::LORA_MOSI,
        BoardPins::LORA_CS);

    LOG_I("RADIO", "Initializing SX1278 on %.3f MHz", LoRaProfile::FREQUENCY_MHZ);
    std::int16_t state = radio_.begin(
        LoRaProfile::FREQUENCY_MHZ,
        LoRaProfile::BANDWIDTH_KHZ,
        LoRaProfile::SPREADING_FACTOR,
        LoRaProfile::CODING_RATE,
        LoRaProfile::SYNC_WORD,
        LoRaProfile::OUTPUT_POWER_DBM,
        LoRaProfile::PREAMBLE_LENGTH,
        LoRaProfile::GAIN);

    if (state != RADIOLIB_ERR_NONE) {
        setError(state);
        LOG_E("RADIO", "Initialization failed: %d", state);
        return false;
    }

    const std::int16_t currentState = radio_.setCurrentLimit(LoRaProfile::CURRENT_LIMIT_MA);
    if (currentState != RADIOLIB_ERR_NONE) {
        LOG_E("RADIO", "Current limit configuration failed: %d", currentState);
    }
    const std::int16_t crcState = radio_.setCRC(false);
    if (crcState != RADIOLIB_ERR_NONE) {
        LOG_E("RADIO", "CRC disable failed: %d", crcState);
    }
    radio_.explicitHeader();

    status_.initialized = true;
    if (!startReceive()) {
        return false;
    }

    status_.lastActivityAtMs = millis();
    LOG_I("RADIO", "Receiver active");
    return true;
}

bool Sx1278Driver::recover() {
    const Status previous = status_;
    const bool ready = begin();
    status_.receivedPackets += previous.receivedPackets;
    status_.transmittedPackets += previous.transmittedPackets;
    status_.receiveErrors += previous.receiveErrors;
    status_.transmitTimeouts += previous.transmitTimeouts;
    if (ready) {
        status_.consecutiveReceiveErrors = 0;
    }
    return ready;
}

void Sx1278Driver::update(std::uint32_t now) {
    if (!status_.initialized) {
        return;
    }

    if (status_.mode == Mode::Transmitting &&
        now - transmitStartedAt_ > AppConfig::RADIO_TX_TIMEOUT_MS) {
        LOG_E("RADIO", "Transmit timeout");
        radio_.finishTransmit();
        ++status_.transmitTimeouts;
        status_.lastActivityAtMs = now;
        setError(RADIOLIB_ERR_TX_TIMEOUT);
        startReceive();
        return;
    }

    if (!interruptFlag_) {
        return;
    }

    noInterrupts();
    interruptFlag_ = false;
    interrupts();

    if (status_.mode == Mode::Transmitting) {
        processTransmitInterrupt();
    } else if (status_.mode == Mode::Receiving) {
        processReceiveInterrupt();
    }
}

bool Sx1278Driver::startTransmit(
    const std::uint8_t* data,
    std::size_t length,
    std::uint32_t now) {

    if (!status_.initialized || status_.mode == Mode::Transmitting ||
        data == nullptr || length == 0 || length > sizeof(transmitBuffer_)) {
        return false;
    }

    std::memcpy(transmitBuffer_, data, length);
    transmitLength_ = length;

    if (interruptAction_ == InterruptAction::Receive) {
        radio_.clearPacketReceivedAction();
        interruptAction_ = InterruptAction::None;
    }
    radio_.setPacketSentAction(onRadioInterrupt);
    interruptAction_ = InterruptAction::Transmit;
    const std::int16_t state = radio_.startTransmit(transmitBuffer_, transmitLength_);
    status_.lastError = state;
    if (state != RADIOLIB_ERR_NONE) {
        setError(state);
        startReceive();
        return false;
    }

    transmitStartedAt_ = now;
    status_.mode = Mode::Transmitting;
    LOG_D("RADIO", "TX started, %u bytes", static_cast<unsigned>(length));
    return true;
}

bool Sx1278Driver::takePacket(Packet& packet) {
    if (!packetAvailable_) {
        return false;
    }
    packet = pendingPacket_;
    packetAvailable_ = false;
    return true;
}

const Sx1278Driver::Status& Sx1278Driver::status() const {
    return status_;
}

bool Sx1278Driver::startReceive() {
    if (interruptAction_ == InterruptAction::Transmit) {
        radio_.clearPacketSentAction();
        interruptAction_ = InterruptAction::None;
    }
    radio_.setPacketReceivedAction(onRadioInterrupt);
    interruptAction_ = InterruptAction::Receive;
    const std::int16_t state = radio_.startReceive();
    status_.lastError = state;
    if (state != RADIOLIB_ERR_NONE) {
        setError(state);
        LOG_E("RADIO", "startReceive failed: %d", state);
        return false;
    }
    status_.mode = Mode::Receiving;
    return true;
}

void Sx1278Driver::processReceiveInterrupt() {
    std::size_t length = radio_.getPacketLength();
    if (length > LoRaProfile::MAX_PACKET_LENGTH) {
        length = LoRaProfile::MAX_PACKET_LENGTH;
    }

    Packet packet;
    packet.length = length;
    const std::int16_t state = radio_.readData(packet.data, packet.length);
    status_.lastError = state;

    if (state == RADIOLIB_ERR_NONE) {
        packet.rssiDbm = radio_.getRSSI();
        packet.snrDb = radio_.getSNR();
        packet.frequencyErrorHz = radio_.getFrequencyError();
        pendingPacket_ = packet;
        packetAvailable_ = true;
        ++status_.receivedPackets;
        status_.consecutiveReceiveErrors = 0;
        status_.lastActivityAtMs = millis();
        LOG_D("RADIO", "RX %u bytes, RSSI %.1f, SNR %.1f",
              static_cast<unsigned>(packet.length), packet.rssiDbm, packet.snrDb);
    } else {
        ++status_.receiveErrors;
        if (status_.consecutiveReceiveErrors < 0xFFU) {
            ++status_.consecutiveReceiveErrors;
        }
        status_.lastActivityAtMs = millis();
        LOG_E("RADIO", "readData failed: %d", state);
    }

    startReceive();
}

void Sx1278Driver::processTransmitInterrupt() {
    const std::int16_t state = radio_.finishTransmit();
    status_.lastError = state;
    if (state == RADIOLIB_ERR_NONE) {
        ++status_.transmittedPackets;
        status_.lastActivityAtMs = millis();
        LOG_D("RADIO", "TX finished");
    } else {
        LOG_E("RADIO", "finishTransmit failed: %d", state);
    }
    startReceive();
}

void Sx1278Driver::setError(std::int16_t error) {
    status_.lastError = error;
    status_.mode = Mode::Error;
}

}  // namespace Drivers
