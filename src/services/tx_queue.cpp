#include "services/tx_queue.h"

#include <cstdio>
#include <cstring>

namespace Services {

void TxQueue::clear() {
    for (Item& item : items_) {
        item = Item{};
    }
    count_ = 0;
    nextSequence_ = 1;
    stats_ = Stats{};
}

bool TxQueue::enqueue(
    const std::uint8_t* data,
    std::size_t length,
    Source source,
    Priority priority,
    std::uint32_t now,
    bool replaceSameSource,
    const char* tagPeer,
    const char* tagId,
    std::uint32_t* sequenceOut) {

    if (data == nullptr || length == 0 || length > LoRaProfile::MAX_PACKET_LENGTH) {
        ++stats_.drops;
        return false;
    }

    if (replaceSameSource) {
        for (std::size_t index = 0; index < count_; ++index) {
            if (items_[index].source != source) {
                continue;
            }
            Item& item = items_[index];
            std::memcpy(item.data, data, length);
            item.length = length;
            item.priority = priority;
            item.queuedAtMs = now;
            std::snprintf(item.tagPeer, sizeof(item.tagPeer), "%s", tagPeer != nullptr ? tagPeer : "");
            std::snprintf(item.tagId, sizeof(item.tagId), "%s", tagId != nullptr ? tagId : "");
            if (sequenceOut != nullptr) {
                *sequenceOut = item.sequence;
            }
            ++stats_.replaced;
            return true;
        }
    }

    if (count_ >= CAPACITY) {
        // A safety-critical/high-priority frame may evict the least important
        // queued frame. Among equal priorities the newest one is discarded.
        std::size_t worst = 0;
        for (std::size_t index = 1; index < count_; ++index) {
            const auto candidatePriority = static_cast<std::uint8_t>(items_[index].priority);
            const auto worstPriority = static_cast<std::uint8_t>(items_[worst].priority);
            if (candidatePriority > worstPriority ||
                (candidatePriority == worstPriority && items_[index].sequence > items_[worst].sequence)) {
                worst = index;
            }
        }
        if (static_cast<std::uint8_t>(priority) >=
            static_cast<std::uint8_t>(items_[worst].priority)) {
            ++stats_.drops;
            return false;
        }
        for (std::size_t index = worst; index + 1 < count_; ++index) {
            items_[index] = items_[index + 1];
        }
        --count_;
        ++stats_.drops;
    }

    Item& item = items_[count_++];
    item = Item{};
    std::memcpy(item.data, data, length);
    item.length = length;
    item.source = source;
    item.priority = priority;
    item.queuedAtMs = now;
    item.sequence = nextSequence_++;
    std::snprintf(item.tagPeer, sizeof(item.tagPeer), "%s", tagPeer != nullptr ? tagPeer : "");
    std::snprintf(item.tagId, sizeof(item.tagId), "%s", tagId != nullptr ? tagId : "");

    if (sequenceOut != nullptr) {
        *sequenceOut = item.sequence;
    }

    ++stats_.enqueued;
    stats_.depth = static_cast<std::uint8_t>(count_);
    if (stats_.depth > stats_.maximumDepth) {
        stats_.maximumDepth = stats_.depth;
    }
    return true;
}

bool TxQueue::peek(Item& item, std::size_t& index) const {
    if (count_ == 0) {
        return false;
    }

    std::size_t best = 0;
    for (std::size_t current = 1; current < count_; ++current) {
        const auto currentPriority = static_cast<std::uint8_t>(items_[current].priority);
        const auto bestPriority = static_cast<std::uint8_t>(items_[best].priority);
        if (currentPriority < bestPriority ||
            (currentPriority == bestPriority && items_[current].sequence < items_[best].sequence)) {
            best = current;
        }
    }
    item = items_[best];
    index = best;
    return true;
}

bool TxQueue::pop(std::size_t index, Item& item) {
    if (index >= count_) {
        return false;
    }
    item = items_[index];
    for (std::size_t current = index; current + 1 < count_; ++current) {
        items_[current] = items_[current + 1];
    }
    --count_;
    items_[count_] = Item{};
    stats_.depth = static_cast<std::uint8_t>(count_);
    return true;
}

bool TxQueue::contains(Source source) const {
    for (std::size_t index = 0; index < count_; ++index) {
        if (items_[index].source == source) {
            return true;
        }
    }
    return false;
}

bool TxQueue::full() const {
    return count_ >= CAPACITY;
}

const TxQueue::Stats& TxQueue::stats() const {
    return stats_;
}

const char* TxQueue::sourceName(Source source) {
    switch (source) {
        case Source::Acknowledgement: return "ACK";
        case Source::Message: return "ZPRAVA";
        case Source::Digipeater: return "DIGI";
        case Source::ManualBeacon: return "RUCNI";
        case Source::Tracker: return "TRACKER";
        case Source::Test: return "TEST";
        default: return "--";
    }
}

}  // namespace Services
