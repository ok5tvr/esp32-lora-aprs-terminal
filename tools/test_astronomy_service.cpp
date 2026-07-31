#include <cassert>
#include <cmath>
#include <iostream>

#include "services/astronomy_service.h"

namespace {

unsigned minutes(const Services::AstronomyService::EventTime& event) {
    return static_cast<unsigned>(event.hour) * 60U + event.minute;
}

}  // namespace

int main() {
    Services::AstronomyService service;
    Services::PositionReference pilsen;
    pilsen.valid = true;
    pilsen.fromGps = true;
    pilsen.latitude = 49.786333;
    pilsen.longitude = 13.285000;
    pilsen.revision = 1;

    service.update(0U, true, 2026, 7, 31, 2026, 7, 31, 10, 0, 0, pilsen);
    const auto summer = service.viewState();
    assert(summer.valid);
    assert(summer.positionFromGps);
    assert(summer.sunrise.valid && summer.sunset.valid);
    assert(minutes(summer.sunrise) >= 5U * 60U + 20U);
    assert(minutes(summer.sunrise) <= 5U * 60U + 50U);
    assert(minutes(summer.sunset) >= 20U * 60U + 35U);
    assert(minutes(summer.sunset) <= 21U * 60U + 5U);
    assert(summer.daylightMinutes >= 890U && summer.daylightMinutes <= 940U);
    assert(summer.moonrise.valid && summer.moonset.valid);
    assert(summer.moonPhase == Services::AstronomyService::MoonPhase::FullMoon);
    assert(summer.moonIlluminationPercent >= 90U);
    assert(summer.moonAgeDays >= 14.0F && summer.moonAgeDays <= 18.0F);
    assert(summer.moonElongationDegrees >= 170.0F && summer.moonElongationDegrees <= 210.0F);
    assert(summer.sunAltitudeDegrees > 45.0F);

    const std::uint32_t firstRevision = summer.revision;
    service.update(60000U, true, 2026, 7, 31, 2026, 7, 31, 10, 1, 0, pilsen);
    assert(service.viewState().revision == firstRevision);
    service.update(300000U, true, 2026, 7, 31, 2026, 7, 31, 10, 5, 0, pilsen);
    assert(service.viewState().revision == firstRevision + 1U);

    const std::uint32_t periodicRevision = service.viewState().revision;
    pilsen.fromGps = false;
    ++pilsen.revision;
    service.update(61000U, true, 2026, 7, 31, 2026, 7, 31, 10, 1, 1, pilsen);
    assert(service.viewState().revision == periodicRevision + 1U);
    assert(!service.viewState().positionFromGps);

    const std::uint32_t sourceRevision = service.viewState().revision;
    pilsen.latitude += 0.001;
    ++pilsen.revision;
    service.update(62000U, true, 2026, 7, 31, 2026, 7, 31, 10, 1, 2, pilsen);
    assert(service.viewState().revision == sourceRevision);

    pilsen.latitude += 0.100;
    ++pilsen.revision;
    service.update(63000U, true, 2026, 7, 31, 2026, 7, 31, 10, 1, 3, pilsen);
    assert(service.viewState().revision == sourceRevision + 1U);

    Services::PositionReference tromso;
    tromso.valid = true;
    tromso.latitude = 69.6492;
    tromso.longitude = 18.9553;
    tromso.revision = 1;
    service.update(64000U, true, 2026, 6, 21, 2026, 6, 21, 10, 0, 0, tromso);
    assert(service.viewState().valid);
    assert(service.viewState().sunAboveAllDay);
    assert(!service.viewState().sunrise.valid);
    assert(!service.viewState().sunset.valid);

    service.update(65000U, false, 0, 0, 0, 0, 0, 0, 0, 0, 0, tromso);
    assert(!service.viewState().valid);
    assert(!service.viewState().timeValid);

    std::cout << "astronomy service tests passed\n";
    return 0;
}
