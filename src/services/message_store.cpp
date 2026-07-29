#include "services/message_store.h"

#include <cstdio>
#include <cstring>

namespace Services {
namespace {

constexpr std::uint8_t MAX_TRANSMIT_ATTEMPTS = 5;
constexpr std::uint32_t RETRY_DELAYS_MS[MAX_TRANSMIT_ATTEMPTS - 1] = {
    10000U,
    30000U,
    60000U,
    120000U
};
constexpr std::uint32_t FINAL_ACK_GRACE_MS = 30000U;

bool timeReached(std::uint32_t now, std::uint32_t target) {
    return static_cast<std::int32_t>(now - target) >= 0;
}

}  // namespace

void MessageStore::clear() {
    view_ = ViewState{};
    ackCount_ = 0;
    nextMessageNumber_ = 0;
}

void MessageStore::ingest(
    const Aprs::ParsedMessage& message,
    const char* ownCallsign,
    std::uint32_t now) {

    if (!message.valid || ownCallsign == nullptr) {
        return;
    }

    const bool addressedToUs = addressEquals(message.addressee, ownCallsign);
    const bool groupAddress = isGroupAddress(message.addressee);
    if (!addressedToUs && !groupAddress) {
        return;
    }

    if (message.kind == Aprs::MessageKind::Acknowledgement ||
        message.kind == Aprs::MessageKind::Rejection) {
        if (!addressedToUs || !message.hasMessageId) {
            return;
        }
        Message* outgoing = findOutgoing(message.source, message.messageId);
        if (outgoing != nullptr) {
            outgoing->state = message.kind == Aprs::MessageKind::Acknowledgement
                ? DeliveryState::Acknowledged
                : DeliveryState::Rejected;
            outgoing->updatedAtMs = now;
            ++view_.revision;
            recountPending();
        }
        return;
    }

    if (addressedToUs && message.hasMessageId) {
        // APRS 1.0.1 requires an acknowledgement for every received copy.
        queueAcknowledgement(message.source, message.messageId);
    }

    if (message.hasMessageId) {
        Message* duplicate = findIncomingDuplicate(message.source, message.messageId);
        if (duplicate != nullptr) {
            duplicate->updatedAtMs = now;
            duplicate->receiveCount = duplicate->receiveCount == 0xFFFFU
                ? 0xFFFFU
                : static_cast<std::uint16_t>(duplicate->receiveCount + 1U);
            std::snprintf(duplicate->text, sizeof(duplicate->text), "%s", message.text);
            ++view_.revision;
            return;
        }
    }

    Message entry;
    entry.direction = Direction::Incoming;
    entry.state = DeliveryState::Received;
    std::snprintf(entry.peer, sizeof(entry.peer), "%s", message.source);
    std::snprintf(entry.text, sizeof(entry.text), "%s", message.text);
    std::snprintf(entry.messageId, sizeof(entry.messageId), "%s", message.messageId);
    entry.hasMessageId = message.hasMessageId;
    entry.groupMessage = groupAddress;
    entry.updatedAtMs = now;
    entry.receiveCount = 1;
    if (insertNewest(entry)) {
        ++view_.receivedMessageEvents;
        ++view_.revision;
    }
}

bool MessageStore::queueOutgoing(
    const char* recipient,
    const char* text,
    std::uint32_t now,
    char* errorText,
    std::size_t errorTextCapacity) {

    char normalized[Aprs::MAX_MESSAGE_ADDRESS_LENGTH + 1] = {};
    if (!normalizeAddress(recipient, normalized, sizeof(normalized))) {
        setError(errorText, errorTextCapacity, "Neplatny adresat. Pouzijte max. 9 znaku A-Z, 0-9 a -.");
        return false;
    }
    if (text == nullptr || text[0] == '\0') {
        setError(errorText, errorTextCapacity, "Text zpravy nesmi byt prazdny.");
        return false;
    }

    char testFrame[160] = {};
    if (!Aprs::buildMessageTnc2(
            "N0CALL",
            "APRS",
            normalized,
            text,
            "000",
            testFrame,
            sizeof(testFrame))) {
        setError(
            errorText,
            errorTextCapacity,
            "Text musi mit max. 67 ASCII znaku a nesmi obsahovat |, ~ nebo {.");
        return false;
    }

    Message entry;
    entry.direction = Direction::Outgoing;
    entry.state = DeliveryState::Pending;
    std::snprintf(entry.peer, sizeof(entry.peer), "%s", normalized);
    std::snprintf(entry.text, sizeof(entry.text), "%s", text);
    createNextMessageId(entry.messageId);
    entry.hasMessageId = true;
    entry.updatedAtMs = now;
    entry.nextAttemptAtMs = now;
    entry.transmitAttempts = 0;

    if (!insertNewest(entry)) {
        setError(errorText, errorTextCapacity, "Seznam je plny nevyresenych odchozich zprav.");
        return false;
    }

    ++view_.revision;
    recountPending();
    setError(errorText, errorTextCapacity, "");
    return true;
}

void MessageStore::update(std::uint32_t now) {
    bool changed = false;
    for (std::size_t index = 0; index < view_.count; ++index) {
        Message& message = view_.messages[index];
        if (message.direction == Direction::Outgoing &&
            message.state == DeliveryState::Pending &&
            message.transmitAttempts >= MAX_TRANSMIT_ATTEMPTS &&
            timeReached(now, message.nextAttemptAtMs)) {
            message.state = DeliveryState::Failed;
            message.updatedAtMs = now;
            changed = true;
        }
    }
    if (changed) {
        ++view_.revision;
        recountPending();
    }
}

bool MessageStore::prepareTransmission(
    const char* ownCallsign,
    const char* destination,
    std::uint32_t now,
    char* frame,
    std::size_t frameCapacity,
    TxToken& token) const {

    token = TxToken{};
    if (ownCallsign == nullptr || destination == nullptr ||
        frame == nullptr || frameCapacity == 0) {
        return false;
    }

    if (ackCount_ > 0) {
        const PendingAck& ack = ackQueue_[0];
        if (!Aprs::buildMessageResponseTnc2(
                ownCallsign,
                destination,
                ack.peer,
                ack.messageId,
                true,
                frame,
                frameCapacity)) {
            return false;
        }
        token.kind = TxToken::Kind::Acknowledgement;
        std::snprintf(token.peer, sizeof(token.peer), "%s", ack.peer);
        std::snprintf(token.messageId, sizeof(token.messageId), "%s", ack.messageId);
        return true;
    }

    for (std::size_t index = 0; index < view_.count; ++index) {
        const Message& message = view_.messages[index];
        if (message.direction != Direction::Outgoing ||
            message.state != DeliveryState::Pending ||
            message.transmitAttempts >= MAX_TRANSMIT_ATTEMPTS ||
            !timeReached(now, message.nextAttemptAtMs)) {
            continue;
        }

        if (!Aprs::buildMessageTnc2(
                ownCallsign,
                destination,
                message.peer,
                message.text,
                message.messageId,
                frame,
                frameCapacity)) {
            return false;
        }
        token.kind = TxToken::Kind::OutgoingMessage;
        std::snprintf(token.peer, sizeof(token.peer), "%s", message.peer);
        std::snprintf(token.messageId, sizeof(token.messageId), "%s", message.messageId);
        return true;
    }
    return false;
}

void MessageStore::markTransmissionStarted(const TxToken& token, std::uint32_t now) {
    if (token.kind == TxToken::Kind::Acknowledgement) {
        removeAcknowledgement(token.peer, token.messageId);
        return;
    }
    if (token.kind != TxToken::Kind::OutgoingMessage) {
        return;
    }

    Message* message = findOutgoing(token.peer, token.messageId);
    if (message == nullptr || message->state != DeliveryState::Pending) {
        return;
    }

    ++message->transmitAttempts;
    message->updatedAtMs = now;
    if (message->transmitAttempts < MAX_TRANSMIT_ATTEMPTS) {
        message->nextAttemptAtMs = now + RETRY_DELAYS_MS[message->transmitAttempts - 1U];
    } else {
        message->nextAttemptAtMs = now + FINAL_ACK_GRACE_MS;
    }
    ++view_.revision;
}

const MessageStore::ViewState& MessageStore::viewState() const {
    return view_;
}

bool MessageStore::addressEquals(const char* first, const char* second) {
    if (first == nullptr || second == nullptr) {
        return false;
    }
    while (*first != '\0' && *second != '\0') {
        char firstValue = *first++;
        char secondValue = *second++;
        if (firstValue >= 'a' && firstValue <= 'z') {
            firstValue = static_cast<char>(firstValue - 'a' + 'A');
        }
        if (secondValue >= 'a' && secondValue <= 'z') {
            secondValue = static_cast<char>(secondValue - 'a' + 'A');
        }
        if (firstValue != secondValue) {
            return false;
        }
    }
    return *first == '\0' && *second == '\0';
}

bool MessageStore::normalizeAddress(
    const char* input,
    char* output,
    std::size_t capacity) {

    if (input == nullptr || output == nullptr ||
        capacity < Aprs::MAX_MESSAGE_ADDRESS_LENGTH + 1) {
        return false;
    }

    std::size_t length = 0;
    for (const char* cursor = input; *cursor != '\0'; ++cursor) {
        char value = *cursor;
        if (value == ' ' || value == '\t') {
            continue;
        }
        if (value >= 'a' && value <= 'z') {
            value = static_cast<char>(value - 'a' + 'A');
        }
        if (!((value >= 'A' && value <= 'Z') ||
              (value >= '0' && value <= '9') || value == '-')) {
            return false;
        }
        if (length >= Aprs::MAX_MESSAGE_ADDRESS_LENGTH) {
            return false;
        }
        output[length++] = value;
    }
    output[length] = '\0';
    return length > 0;
}

bool MessageStore::isGroupAddress(const char* address) {
    return addressEquals(address, "ALL") ||
        addressEquals(address, "QST") ||
        addressEquals(address, "CQ");
}

void MessageStore::setError(char* output, std::size_t capacity, const char* text) {
    if (output != nullptr && capacity > 0) {
        std::snprintf(output, capacity, "%s", text != nullptr ? text : "");
    }
}

MessageStore::Message* MessageStore::findOutgoing(
    const char* peer,
    const char* messageId) {

    for (std::size_t index = 0; index < view_.count; ++index) {
        Message& message = view_.messages[index];
        if (message.direction == Direction::Outgoing &&
            message.hasMessageId &&
            addressEquals(message.peer, peer) &&
            std::strcmp(message.messageId, messageId) == 0) {
            return &message;
        }
    }
    return nullptr;
}

MessageStore::Message* MessageStore::findIncomingDuplicate(
    const char* peer,
    const char* messageId) {

    for (std::size_t index = 0; index < view_.count; ++index) {
        Message& message = view_.messages[index];
        if (message.direction == Direction::Incoming &&
            message.hasMessageId &&
            addressEquals(message.peer, peer) &&
            std::strcmp(message.messageId, messageId) == 0) {
            if (index > 0) {
                moveToFront(index);
                return &view_.messages[0];
            }
            return &message;
        }
    }
    return nullptr;
}

bool MessageStore::insertNewest(const Message& message) {
    std::size_t removeIndex = view_.count;
    if (view_.count >= MAX_MESSAGES) {
        for (std::size_t index = view_.count; index > 0; --index) {
            const std::size_t candidate = index - 1;
            if (view_.messages[candidate].state != DeliveryState::Pending) {
                removeIndex = candidate;
                break;
            }
        }
        if (removeIndex >= view_.count) {
            return false;
        }
    }

    if (view_.count < MAX_MESSAGES) {
        ++view_.count;
        removeIndex = view_.count - 1;
    }

    for (std::size_t index = removeIndex; index > 0; --index) {
        view_.messages[index] = view_.messages[index - 1];
    }
    view_.messages[0] = message;
    return true;
}

void MessageStore::moveToFront(std::size_t index) {
    if (index == 0 || index >= view_.count) {
        return;
    }
    const Message saved = view_.messages[index];
    for (std::size_t current = index; current > 0; --current) {
        view_.messages[current] = view_.messages[current - 1];
    }
    view_.messages[0] = saved;
}

void MessageStore::queueAcknowledgement(const char* peer, const char* messageId) {
    if (peer == nullptr || messageId == nullptr || messageId[0] == '\0') {
        return;
    }
    char normalizedPeer[Aprs::MAX_MESSAGE_ADDRESS_LENGTH + 1] = {};
    if (!normalizeAddress(peer, normalizedPeer, sizeof(normalizedPeer))) {
        return;
    }
    for (std::size_t index = 0; index < ackCount_; ++index) {
        if (addressEquals(ackQueue_[index].peer, normalizedPeer) &&
            std::strcmp(ackQueue_[index].messageId, messageId) == 0) {
            return;
        }
    }

    if (ackCount_ >= MAX_ACK_QUEUE) {
        for (std::size_t index = 1; index < ackCount_; ++index) {
            ackQueue_[index - 1] = ackQueue_[index];
        }
        --ackCount_;
    }
    std::snprintf(ackQueue_[ackCount_].peer, sizeof(ackQueue_[ackCount_].peer), "%s", normalizedPeer);
    std::snprintf(
        ackQueue_[ackCount_].messageId,
        sizeof(ackQueue_[ackCount_].messageId),
        "%s",
        messageId);
    ++ackCount_;
}

void MessageStore::removeAcknowledgement(const char* peer, const char* messageId) {
    for (std::size_t index = 0; index < ackCount_; ++index) {
        if (!addressEquals(ackQueue_[index].peer, peer) ||
            std::strcmp(ackQueue_[index].messageId, messageId) != 0) {
            continue;
        }
        for (std::size_t next = index + 1; next < ackCount_; ++next) {
            ackQueue_[next - 1] = ackQueue_[next];
        }
        --ackCount_;
        return;
    }
}

void MessageStore::recountPending() {
    std::size_t count = 0;
    for (std::size_t index = 0; index < view_.count; ++index) {
        if (view_.messages[index].direction == Direction::Outgoing &&
            view_.messages[index].state == DeliveryState::Pending) {
            ++count;
        }
    }
    view_.pendingOutgoing = count;
}

bool MessageStore::messageIdInUse(const char* messageId) const {
    for (std::size_t index = 0; index < view_.count; ++index) {
        const Message& message = view_.messages[index];
        if (message.direction == Direction::Outgoing &&
            message.state == DeliveryState::Pending &&
            std::strcmp(message.messageId, messageId) == 0) {
            return true;
        }
    }
    return false;
}

void MessageStore::createNextMessageId(
    char output[Aprs::MAX_MESSAGE_ID_LENGTH + 1]) {

    for (std::size_t attempt = 0; attempt < 1000; ++attempt) {
        std::snprintf(output, Aprs::MAX_MESSAGE_ID_LENGTH + 1, "%03u",
            static_cast<unsigned>(nextMessageNumber_ % 1000U));
        nextMessageNumber_ = static_cast<std::uint16_t>((nextMessageNumber_ + 1U) % 1000U);
        if (!messageIdInUse(output)) {
            return;
        }
    }
    std::snprintf(output, Aprs::MAX_MESSAGE_ID_LENGTH + 1, "999");
}

}  // namespace Services
