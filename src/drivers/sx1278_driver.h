#pragma once

#include <RadioLib.h>
#include <SPI.h>
#include <cstddef>
#include <cstdint>

#include "lora_profile.h"

namespace Drivers {

class Sx1278Driver {
public:
    enum class Mode : std::uint8_t {
        Offline,
        Receiving,
        Transmitting,
        Error
    };

    enum class InterruptAction : std::uint8_t {
        None,
        Receive,
        Transmit
    };

    struct Packet {
        std::uint8_t data[LoRaProfile::MAX_PACKET_LENGTH] = {};
        std::size_t length = 0;
        float rssiDbm = 0.0F;
        float snrDb = 0.0F;
        float frequencyErrorHz = 0.0F;
    };

    struct Status {
        bool initialized = false;
        Mode mode = Mode::Offline;
        std::int16_t lastError = RADIOLIB_ERR_NONE;
        std::uint32_t receivedPackets = 0;
        std::uint32_t transmittedPackets = 0;
        std::uint32_t receiveErrors = 0;
        std::uint32_t transmitTimeouts = 0;
        std::uint8_t consecutiveReceiveErrors = 0;
        std::uint32_t lastActivityAtMs = 0;
    };

    Sx1278Driver();

    bool begin(const LoRaProfile::Config& config);
    bool reconfigure(const LoRaProfile::Config& config);
    bool recover();
    void update(std::uint32_t now);
    bool startTransmit(const std::uint8_t* data, std::size_t length, std::uint32_t now);
    bool takePacket(Packet& packet);
    bool readCurrentRssi(float& rssiDbm);
    const Status& status() const;
    const LoRaProfile::Config& config() const;

private:
    static Sx1278Driver* activeInstance_;
    static void onRadioInterrupt();

    bool initialize();
    static void preserveCounters(const Status& previous, Status& current);
    bool startReceive();
    void processReceiveInterrupt();
    void processTransmitInterrupt();
    void setError(std::int16_t error);

    SPIClass spi_;
    SPISettings spiSettings_;
    Module module_;
    SX1278 radio_;

    volatile bool interruptFlag_ = false;
    LoRaProfile::Config config_ = LoRaProfile::czeAprsConfig();
    Status status_;
    Packet pendingPacket_;
    bool packetAvailable_ = false;
    std::uint8_t transmitBuffer_[LoRaProfile::MAX_PACKET_LENGTH] = {};
    std::size_t transmitLength_ = 0;
    std::uint32_t transmitStartedAt_ = 0;
    InterruptAction interruptAction_ = InterruptAction::None;
};

}  // namespace Drivers
