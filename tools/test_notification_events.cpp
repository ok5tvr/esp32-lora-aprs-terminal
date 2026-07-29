#include <cassert>
#include <cstdio>
#include <cstring>

#include "services/message_store.h"
#include "services/station_store.h"

int main() {
    Services::StationStore stations;
    stations.clear();

    Aprs::ParsedFrame station;
    station.valid = true;
    station.type = Aprs::EntityType::Station;
    station.alive = true;
    std::snprintf(station.source, sizeof(station.source), "OK1AAA-1");
    std::snprintf(station.entityName, sizeof(station.entityName), "OK1AAA-1");
    assert(stations.ingest(station, -80.0F, 7.0F, 100U));
    assert(stations.viewState().discoveredEntities == 1U);

    // A repeated packet updates history but is not a newly discovered entity.
    assert(stations.ingest(station, -79.0F, 8.0F, 200U));
    assert(stations.viewState().discoveredEntities == 1U);

    std::snprintf(station.source, sizeof(station.source), "OK1BBB-2");
    std::snprintf(station.entityName, sizeof(station.entityName), "OK1BBB-2");
    assert(stations.ingest(station, -70.0F, 9.0F, 300U));
    assert(stations.viewState().discoveredEntities == 2U);

    Services::MessageStore messages;
    messages.clear();

    Aprs::ParsedMessage message;
    message.valid = true;
    message.kind = Aprs::MessageKind::Text;
    message.hasMessageId = true;
    std::snprintf(message.source, sizeof(message.source), "OK1AAA-1");
    std::snprintf(message.addressee, sizeof(message.addressee), "OK5TVR-15");
    std::snprintf(message.text, sizeof(message.text), "TEST");
    std::snprintf(message.messageId, sizeof(message.messageId), "001");
    messages.ingest(message, "OK5TVR-15", 100U);
    assert(messages.viewState().receivedMessageEvents == 1U);

    // Duplicate identified message must not create another unread event.
    messages.ingest(message, "OK5TVR-15", 200U);
    assert(messages.viewState().receivedMessageEvents == 1U);

    std::snprintf(message.messageId, sizeof(message.messageId), "002");
    messages.ingest(message, "OK5TVR-15", 300U);
    assert(messages.viewState().receivedMessageEvents == 2U);

    std::puts("notification event tests passed");
    return 0;
}
