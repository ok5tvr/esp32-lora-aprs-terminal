#include "services/digi_igate_service.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

int main() {
    Services::DigiIgateService service;
    assert(service.begin());

    Services::SettingsService::ViewState settings;
    std::strcpy(settings.callsign, "OK5TVR-17");
    settings.digiEnabled = true;
    settings.digiMode = App::DigiMode::FillInAndWide2;
    settings.digiMaxWideHops = 2;
    service.update(0, settings);

    const char* frame = "OK1ABC>APRS,WIDE1-1:>hello";
    service.ingestRfFrame(
        reinterpret_cast<const std::uint8_t*>(frame),
        std::strlen(frame),
        0);
    assert(service.viewState().digiQueueDepth == 1);

    std::uint8_t repeated[256] = {};
    std::size_t repeatedLength = 0;
    assert(!service.takeDigiFrame(100, repeated, sizeof(repeated), repeatedLength));
    assert(service.takeDigiFrame(1000, repeated, sizeof(repeated), repeatedLength));
    assert(std::string(reinterpret_cast<const char*>(repeated), repeatedLength) ==
        "OK1ABC>APRS,OK5TVR-17*:>hello");
    service.markDigiTransmitResult(true);
    assert(service.viewState().digipeatedPackets == 1);

    service.ingestRfFrame(
        reinterpret_cast<const std::uint8_t*>(frame),
        std::strlen(frame),
        2000);
    assert(service.viewState().digiQueueDepth == 0);
    assert(service.viewState().digiDuplicates == 1);

    settings.igateEnabled = true;
    std::strcpy(settings.wifiSsid, "test");
    std::strcpy(settings.aprsIsServer, "rotate.aprs2.net");
    settings.aprsIsPort = 14580;
    settings.aprsIsPasscode = 12345;
    service.update(3000, settings);
    service.ingestRfFrame(
        reinterpret_cast<const std::uint8_t*>(frame),
        std::strlen(frame),
        4000);
    assert(service.viewState().igateQueueDepth == 1);

    const char* noGate = "OK1ABC>APRS,NOGATE:>private";
    service.ingestRfFrame(
        reinterpret_cast<const std::uint8_t*>(noGate),
        std::strlen(noGate),
        5000);
    assert(service.viewState().igateQueueDepth == 1);
    assert(service.viewState().gateFiltered >= 1);

    std::cout << "digi/igate service tests passed\n";
    return 0;
}
