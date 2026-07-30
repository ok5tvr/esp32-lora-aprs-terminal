#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "Arduino.h"
#include "SD.h"
#include "drivers/sd_card_driver.h"
#include "services/trail_service.h"

SerialStub Serial;
SDClass SD;

namespace FakeFs {
std::vector<Entry> entries;
int find(const char* path) {
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].path == path) return static_cast<int>(i);
    }
    return -1;
}
}

bool File::isDirectory() const { return valid_ && FakeFs::entries[index_].directory; }
const char* File::name() const { return valid_ ? FakeFs::entries[index_].path.c_str() : ""; }
std::size_t File::size() const { return valid_ ? FakeFs::entries[index_].data.size() : 0; }
File File::openNextFile() {
    if (!isDirectory()) return File();
    const std::string prefix = FakeFs::entries[index_].path + "/";
    while (iterator_ < FakeFs::entries.size()) {
        const std::size_t i = iterator_++;
        if (FakeFs::entries[i].path.rfind(prefix, 0) == 0 &&
            FakeFs::entries[i].path.find('/', prefix.size()) == std::string::npos) {
            return File(static_cast<int>(i));
        }
    }
    return File();
}
std::size_t File::print(const char* text) {
    return write(reinterpret_cast<const std::uint8_t*>(text), std::strlen(text));
}
std::size_t File::write(const std::uint8_t* data, std::size_t length) {
    if (!valid_ || isDirectory()) return 0;
    FakeFs::entries[index_].data.append(reinterpret_cast<const char*>(data), length);
    return length;
}

bool SDClass::exists(const char* path) { return FakeFs::find(path) >= 0; }
bool SDClass::mkdir(const char* path) {
    if (!exists(path)) FakeFs::entries.push_back({path, "", true});
    return true;
}
File SDClass::open(const char* path, int mode) {
    int index = FakeFs::find(path);
    if (index < 0 && mode == FILE_WRITE) {
        FakeFs::entries.push_back({path, "", false});
        index = static_cast<int>(FakeFs::entries.size() - 1);
    }
    return File(index);
}
const std::string& SDClass::content(const char* path) const {
    static const std::string empty;
    const int index = FakeFs::find(path);
    return index >= 0 ? FakeFs::entries[index].data : empty;
}

namespace Drivers { namespace SdCard {
Status current{true, 1, 0, 0, 0};
const Status& status() { return current; }
bool begin() { return current.mounted; }
void refreshUsage() {}
} }

static Services::GpsService::ViewState gps(double lat, double lon, float speed, unsigned sec) {
    Services::GpsService::ViewState g;
    g.hasFix = true;
    g.utcDateValid = true;
    g.utcTimeValid = true;
    g.latitude = lat;
    g.longitude = lon;
    g.altitudeMeters = 350.0;
    g.speedValid = true;
    g.speedKmh = speed;
    g.courseValid = true;
    g.courseDegrees = 90.0F;
    g.satellites = 8;
    g.hdop = 1.1F;
    g.utcYear = 2026;
    g.utcMonth = 7;
    g.utcDay = 29;
    g.utcHour = 12;
    g.utcMinute = 0;
    g.utcSecond = static_cast<std::uint8_t>(sec % 60);
    return g;
}

int main() {
    Services::TrailService trail;
    assert(trail.begin());

    auto moving = gps(49.786333, 13.285000, 10.0F, 1);
    trail.update(1000, false, moving);
    assert(trail.viewState().state == Services::TrailService::State::Disabled);

    trail.update(1100, true, moving);
    trail.update(1102, true, moving);
    trail.update(1104, true, moving);
    assert(trail.viewState().state == Services::TrailService::State::Recording);
    assert(trail.viewState().fileOpen);
    assert(trail.viewState().pointsWritten == 1);
    assert(trail.viewState().logCount == 1);

    auto stopped = gps(49.786333, 13.285000, 0.0F, 2);
    trail.update(2000, true, stopped);
    trail.update(32000, true, stopped);
    assert(trail.viewState().state == Services::TrailService::State::AutoPaused);
    assert(trail.viewState().autoPaused);

    char message[128] = {};
    assert(trail.toggleManualPause(32500, stopped, message, sizeof(message)));
    assert(trail.viewState().manualPaused);
    trail.update(33500, true, stopped);
    assert(trail.viewState().elapsedSeconds >= 32);
    assert(trail.toggleManualPause(34000, stopped, message, sizeof(message)));
    assert(!trail.viewState().manualPaused);

    auto movedAgain = gps(49.786333, 13.285150, 3.0F, 35);
    trail.update(35000, true, movedAgain);
    assert(trail.viewState().state == Services::TrailService::State::Recording);
    assert(trail.viewState().recentPointCount >= 2U);
    const std::uint8_t recentBeforeClose = trail.viewState().recentPointCount;

    const std::string active = trail.viewState().activeFile;
    trail.update(40000, false, movedAgain);
    assert(trail.viewState().state == Services::TrailService::State::Disabled);
    assert(!trail.viewState().fileOpen);
    assert(trail.viewState().recentPointCount == recentBeforeClose);

    const std::string path = std::string("/STOPAR/") + active;
    const std::string& log = SD.content(path.c_str());
    assert(log.find("# LoRa APRS Terminal - Stopar") != std::string::npos);
    assert(log.find("AUTO_PAUSE") != std::string::npos);
    assert(log.find("MANUAL_PAUSE") != std::string::npos);
    assert(log.find("MANUAL_RESUME") != std::string::npos);
    assert(log.find(";49.786333;13.285000;") != std::string::npos);
    std::cout << "trail service tests passed\n";
}
