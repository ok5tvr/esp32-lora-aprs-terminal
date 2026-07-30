#pragma once

#include <aprs_codec.h>
#include <cstddef>
#include <cstdint>

namespace Services {

class MessageStore {
public:
    static constexpr std::size_t MAX_MESSAGES = 20;
    static constexpr std::size_t MAX_ACK_QUEUE = 6;

    enum class Direction : std::uint8_t {
        Incoming,
        Outgoing
    };

    enum class DeliveryState : std::uint8_t {
        Received,
        Pending,
        Acknowledged,
        Rejected,
        Failed
    };

    struct Message {
        Direction direction = Direction::Incoming;
        DeliveryState state = DeliveryState::Received;
        char peer[Aprs::MAX_SOURCE_CALL_LENGTH + 1] = {};
        char text[Aprs::MAX_MESSAGE_TEXT_LENGTH + 1] = {};
        char messageId[Aprs::MAX_MESSAGE_ID_LENGTH + 1] = {};
        bool hasMessageId = false;
        bool groupMessage = false;
        std::uint8_t transmitAttempts = 0;
        std::uint16_t receiveCount = 1;
        std::uint32_t updatedAtMs = 0;
        std::uint32_t nextAttemptAtMs = 0;
    };

    struct ViewState {
        Message messages[MAX_MESSAGES] = {};
        std::size_t count = 0;
        std::size_t pendingOutgoing = 0;
        // Counts newly inserted incoming APRS messages. Repeated copies with
        // the same source and message ID do not increment this counter.
        std::uint32_t receivedMessageEvents = 0;
        std::uint32_t revision = 0;
    };

    struct TxToken {
        enum class Kind : std::uint8_t {
            None,
            Acknowledgement,
            OutgoingMessage
        };

        Kind kind = Kind::None;
        char peer[Aprs::MAX_SOURCE_CALL_LENGTH + 1] = {};
        char messageId[Aprs::MAX_MESSAGE_ID_LENGTH + 1] = {};
    };

    void clear();
    void ingest(
        const Aprs::ParsedMessage& message,
        const char* ownCallsign,
        std::uint32_t now);
    bool queueOutgoing(
        const char* recipient,
        const char* text,
        std::uint32_t now,
        char* errorText,
        std::size_t errorTextCapacity);
    void update(std::uint32_t now);
    bool prepareTransmission(
        const char* ownCallsign,
        const char* destination,
        std::uint32_t now,
        char* frame,
        std::size_t frameCapacity,
        TxToken& token) const;
    void markTransmissionStarted(const TxToken& token, std::uint32_t now);
    const ViewState& viewState() const;

private:
    struct PendingAck {
        char peer[Aprs::MAX_SOURCE_CALL_LENGTH + 1] = {};
        char messageId[Aprs::MAX_MESSAGE_ID_LENGTH + 1] = {};
    };

    static bool addressEquals(const char* first, const char* second);
    static bool normalizeAddress(const char* input, char* output, std::size_t capacity);
    static bool isGroupAddress(const char* address);
    static void setError(char* output, std::size_t capacity, const char* text);

    Message* findOutgoing(const char* peer, const char* messageId);
    Message* findIncomingDuplicate(const char* peer, const char* messageId);
    bool insertNewest(const Message& message);
    void moveToFront(std::size_t index);
    void queueAcknowledgement(const char* peer, const char* messageId);
    void removeAcknowledgement(const char* peer, const char* messageId);
    void recountPending();
    bool messageIdInUse(const char* messageId) const;
    void createNextMessageId(char output[Aprs::MAX_MESSAGE_ID_LENGTH + 1]);

    ViewState view_;
    PendingAck ackQueue_[MAX_ACK_QUEUE] = {};
    std::size_t ackCount_ = 0;
    std::uint16_t nextMessageNumber_ = 0;
};

}  // namespace Services
