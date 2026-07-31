#include <cassert>
#include <cstdint>
#include <cstring>

#include "Wire.h"
#include "services/time_service.h"

TwoWire Wire;
unsigned long millis() { return 0; }

namespace {
std::uint8_t bcd(unsigned value) {
    return static_cast<std::uint8_t>(((value / 10U) << 4U) | (value % 10U));
}

void setRtc(std::uint16_t year, std::uint8_t month, std::uint8_t day,
            std::uint8_t hour, std::uint8_t minute, std::uint8_t second) {
    Wire.registers[0x00] = 0;
    Wire.registers[0x04] = bcd(second);
    Wire.registers[0x05] = bcd(minute);
    Wire.registers[0x06] = bcd(hour);
    Wire.registers[0x07] = bcd(day);
    Wire.registers[0x08] = 0;
    Wire.registers[0x09] = bcd(month);
    Wire.registers[0x0A] = bcd(year % 100U);
}
}

int main() {
    setRtc(2026, 1, 15, 12, 0, 0);
    Services::TimeService service;
    assert(service.begin(1000));
    auto state = service.viewState();
    assert(state.valid);
    assert(state.source == Services::TimeService::Source::Rtc);
    assert(state.localHour == 13);
    assert(state.localMinute == 0);
    assert(std::strcmp(state.timezone, "CET") == 0);

    Services::GpsService::ViewState gps;
    gps.utcDateValid = true;
    gps.utcTimeValid = true;
    gps.utcYear = 2026;
    gps.utcMonth = 7;
    gps.utcDay = 31;
    gps.utcHour = 8;
    gps.utcMinute = 18;
    gps.utcSecond = 0;
    service.update(2000, gps);
    state = service.viewState();
    assert(state.source == Services::TimeService::Source::Gps);
    assert(state.localHour == 10);
    assert(state.localMinute == 18);
    assert(std::strcmp(state.timezone, "CEST") == 0);
    assert(Wire.registers[0x04] == bcd(0));
    assert(Wire.registers[0x05] == bcd(18));
    assert(Wire.registers[0x06] == bcd(8));
    assert(Wire.registers[0x07] == bcd(31));
    assert(Wire.registers[0x09] == bcd(7));
    assert(Wire.registers[0x0A] == bcd(26));

    // Simulate an invalid RTC. It must not be polled on every main-loop pass.
    TwoWire invalidWire;
    Wire = invalidWire;
    Wire.registers[0x00] = 0;
    Wire.registers[0x04] = 0x80;  // oscillator-stop/invalid flag
    Services::TimeService invalidRtcService;
    assert(invalidRtcService.begin(1000));
    const std::uint32_t readsAfterBegin = Wire.requestCount;
    Services::GpsService::ViewState noGps;
    invalidRtcService.update(1100, noGps);
    invalidRtcService.update(1200, noGps);
    assert(Wire.requestCount == readsAfterBegin);
    invalidRtcService.update(61000, noGps);
    assert(Wire.requestCount > readsAfterBegin);
    return 0;
}
