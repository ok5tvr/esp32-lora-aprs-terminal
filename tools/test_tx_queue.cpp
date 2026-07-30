#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "services/tx_queue.h"

namespace {

void enqueueOne(
    Services::TxQueue& queue,
    char value,
    Services::TxQueue::Source source,
    Services::TxQueue::Priority priority,
    std::uint32_t now) {

    const std::uint8_t data[] = {static_cast<std::uint8_t>(value)};
    assert(queue.enqueue(data, sizeof(data), source, priority, now));
}

}  // namespace

int main() {
    Services::TxQueue queue;
    queue.clear();
    const std::uint8_t tracker1[] = {'T','1'};
    const std::uint8_t tracker2[] = {'T','2'};
    const std::uint8_t ack[] = {'A'};

    // A newer scheduled tracker replaces a stale queued position.
    assert(queue.enqueue(
        tracker1, sizeof(tracker1),
        Services::TxQueue::Source::Tracker,
        Services::TxQueue::Priority::Tracker,
        100U, true));
    assert(queue.enqueue(
        tracker2, sizeof(tracker2),
        Services::TxQueue::Source::Tracker,
        Services::TxQueue::Priority::Tracker,
        200U, true));
    assert(queue.stats().depth == 1U);
    assert(queue.stats().replaced == 1U);

    assert(queue.enqueue(
        ack, sizeof(ack),
        Services::TxQueue::Source::Acknowledgement,
        Services::TxQueue::Priority::Acknowledgement,
        300U));

    Services::TxQueue::Item item;
    std::size_t index = 0;
    assert(queue.peek(item, index));
    assert(item.source == Services::TxQueue::Source::Acknowledgement);
    assert(queue.pop(index, item));
    assert(queue.peek(item, index));
    assert(item.source == Services::TxQueue::Source::Tracker);
    assert(item.length == 2U && item.data[1] == '2');

    // All priority classes must be selected deterministically. Equal-priority
    // traffic remains FIFO, even if a later item has an earlier timestamp.
    queue.clear();
    enqueueOne(queue, 'x', Services::TxQueue::Source::Test,
               Services::TxQueue::Priority::Test, 100U);
    enqueueOne(queue, 't', Services::TxQueue::Source::Tracker,
               Services::TxQueue::Priority::Tracker, 110U);
    enqueueOne(queue, 'm', Services::TxQueue::Source::ManualBeacon,
               Services::TxQueue::Priority::ManualBeacon, 120U);
    enqueueOne(queue, 'd', Services::TxQueue::Source::Digipeater,
               Services::TxQueue::Priority::Digipeater, 130U);
    enqueueOne(queue, '1', Services::TxQueue::Source::Message,
               Services::TxQueue::Priority::Message, 140U);
    enqueueOne(queue, '2', Services::TxQueue::Source::Message,
               Services::TxQueue::Priority::Message, 10U);
    enqueueOne(queue, 'a', Services::TxQueue::Source::Acknowledgement,
               Services::TxQueue::Priority::Acknowledgement, 150U);

    const char expected[] = {'a', '1', '2', 'd', 'm', 't', 'x'};
    for (char value : expected) {
        assert(queue.peek(item, index));
        assert(queue.pop(index, item));
        assert(item.length == 1U && item.data[0] == static_cast<std::uint8_t>(value));
    }
    assert(queue.stats().depth == 0U);

    // A high-priority ACK may evict one low-priority frame when full.
    queue.clear();
    const std::uint8_t test[] = {'X'};
    for (std::size_t i = 0; i < Services::TxQueue::CAPACITY; ++i) {
        assert(queue.enqueue(
            test, sizeof(test),
            Services::TxQueue::Source::Test,
            Services::TxQueue::Priority::Test,
            static_cast<std::uint32_t>(i)));
    }
    assert(queue.enqueue(
        ack, sizeof(ack),
        Services::TxQueue::Source::Acknowledgement,
        Services::TxQueue::Priority::Acknowledgement,
        1000U));
    assert(queue.stats().depth == Services::TxQueue::CAPACITY);
    assert(queue.stats().drops == 1U);
    assert(queue.peek(item, index));
    assert(item.source == Services::TxQueue::Source::Acknowledgement);

    std::puts("tx_queue tests passed");
    return 0;
}
