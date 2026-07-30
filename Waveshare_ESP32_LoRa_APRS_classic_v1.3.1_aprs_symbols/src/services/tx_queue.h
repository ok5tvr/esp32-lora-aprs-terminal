#pragma once

#include <cstddef>
#include <cstdint>

#include "lora_profile.h"

namespace Services {

class TxQueue {
public:
    static constexpr std::size_t CAPACITY = 8;

    enum class Source : std::uint8_t {
        Acknowledgement,
        Message,
        Digipeater,
        ManualBeacon,
        Tracker,
        Test
    };

    enum class Priority : std::uint8_t {
        Acknowledgement = 0,
        Message = 1,
        Digipeater = 2,
        ManualBeacon = 3,
        Tracker = 4,
        Test = 5
    };

    struct Item {
        std::uint8_t data[LoRaProfile::MAX_PACKET_LENGTH] = {};
        std::size_t length = 0;
        Source source = Source::Test;
        Priority priority = Priority::Test;
        std::uint32_t queuedAtMs = 0;
        std::uint32_t sequence = 0;
        char tagPeer[16] = {};
        char tagId[8] = {};
    };

    struct Stats {
        std::uint8_t depth = 0;
        std::uint8_t maximumDepth = 0;
        std::uint32_t enqueued = 0;
        std::uint32_t replaced = 0;
        std::uint32_t drops = 0;
    };

    void clear();
    bool enqueue(
        const std::uint8_t* data,
        std::size_t length,
        Source source,
        Priority priority,
        std::uint32_t now,
        bool replaceSameSource = false,
        const char* tagPeer = nullptr,
        const char* tagId = nullptr);
    bool peek(Item& item, std::size_t& index) const;
    bool pop(std::size_t index, Item& item);
    bool contains(Source source) const;
    bool full() const;
    const Stats& stats() const;

    static const char* sourceName(Source source);

private:
    Item items_[CAPACITY] = {};
    std::size_t count_ = 0;
    std::uint32_t nextSequence_ = 1;
    Stats stats_;
};

}  // namespace Services
