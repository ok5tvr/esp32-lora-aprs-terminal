#include <cassert>
#include <cstring>
#include <iostream>

#include <aprs_codec.h>
#include "services/station_store.h"

int main() {
    Services::StationStore store;
    Aprs::ParsedFrame frame;

    assert(Aprs::parseTnc2(
        "OK1ABC-7>APRS,WIDE1-1,WIDE2-1:!4900.00N/01400.00E>direct",
        frame));
    assert(frame.path.direct);
    assert(store.ingest(frame, -100.0F, 7.0F, 1000U, "direct"));

    assert(Aprs::parseTnc2(
        "OK1ABC-7>APRS,OK0AAA-2*,WIDE2-1*:!4900.00N/01400.00E>repeat",
        frame));
    assert(!frame.path.direct);
    assert(frame.path.digipeaterHops == 2U);
    assert(std::strcmp(frame.path.lastDigipeater, "WIDE2-1") == 0);
    assert(store.ingest(frame, -110.0F, 3.0F, 5000U, "repeat"));

    const auto& view = store.viewState();
    assert(view.count == 1U);
    const auto& station = view.stations[0];
    assert(station.heardCount == 2U);
    assert(station.directReceptionCount == 1U);
    assert(station.repeatedReceptionCount == 1U);
    assert(station.hasDirectReception);
    assert(station.lastDirectHeardMs == 1000U);
    assert(!station.lastReceptionDirect);
    assert(station.digipeaterHops == 2U);
    assert(std::strcmp(station.path, "OK0AAA-2*,WIDE2-1*") == 0);
    assert(std::strcmp(station.lastDigipeater, "WIDE2-1") == 0);

    assert(Aprs::parseTnc2(
        "OK1ABC-7>APRS:!4900.00N/01400.00E>direct again",
        frame));
    assert(store.ingest(frame, -99.0F, 8.0F, 9000U, "direct again"));
    const auto& updated = store.viewState().stations[0];
    assert(updated.heardCount == 3U);
    assert(updated.directReceptionCount == 2U);
    assert(updated.repeatedReceptionCount == 1U);
    assert(updated.lastReceptionDirect);
    assert(updated.lastDirectHeardMs == 9000U);
    assert(updated.path[0] == '\0');

    std::cout << "station route analysis tests passed\n";
    return 0;
}
