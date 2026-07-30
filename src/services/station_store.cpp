#include "services/station_store.h"

#include <cstdio>
#include <cstring>

namespace Services {

void StationStore::clear() {
    view_ = ViewState{};
}

bool StationStore::ingest(
    const Aprs::ParsedFrame& frame,
    float rssiDbm,
    float snrDb,
    std::uint32_t now,
    const char* lastFrame) {

    if (!frame.valid || frame.source[0] == '\0' || frame.entityName[0] == '\0') {
        return false;
    }

    std::size_t existingIndex = view_.count;
    for (std::size_t index = 0; index < view_.count; ++index) {
        if (sameIdentity(view_.stations[index], frame)) {
            existingIndex = index;
            break;
        }
    }

    const bool isNewEntity = existingIndex >= view_.count;

    // APRS kill reports remove live objects/items from the visible history.
    if (frame.type != Aprs::EntityType::Station && !frame.alive) {
        if (existingIndex < view_.count) {
            removeAt(existingIndex);
            ++view_.revision;
        }
        return true;
    }

    Station updated;
    if (existingIndex < view_.count) {
        updated = view_.stations[existingIndex];
        for (std::size_t index = existingIndex; index + 1 < view_.count; ++index) {
            view_.stations[index] = view_.stations[index + 1];
        }
    } else if (view_.count < MAX_STATIONS) {
        ++view_.count;
    }

    for (std::size_t index = view_.count - 1; index > 0; --index) {
        view_.stations[index] = view_.stations[index - 1];
    }

    updated.used = true;
    updated.type = frame.type;
    std::strncpy(updated.callsign, frame.source, sizeof(updated.callsign) - 1);
    updated.callsign[sizeof(updated.callsign) - 1] = '\0';
    std::strncpy(updated.entityName, frame.entityName, sizeof(updated.entityName) - 1);
    updated.entityName[sizeof(updated.entityName) - 1] = '\0';
    updated.positionFormat = frame.positionFormat;
    updated.emergency = frame.emergency;
    updated.telemetry = frame.telemetry;
    updated.phg = frame.phg;
    updated.frequency = frame.frequency;
    updated.lastRssiDbm = rssiDbm;
    updated.lastSnrDb = snrDb;
    updated.lastHeardMs = now;
    ++updated.heardCount;
    if (lastFrame != nullptr && lastFrame[0] != '\0') {
        std::snprintf(updated.lastFrame, sizeof(updated.lastFrame), "%s", lastFrame);
    }

    if (frame.hasPosition) {
        updated.hasPosition = true;
        updated.latitude = frame.latitude;
        updated.longitude = frame.longitude;
        updated.symbol[0] = frame.symbolTable;
        updated.symbol[1] = frame.symbolCode;
        updated.symbol[2] = '\0';
    }

    view_.stations[0] = updated;
    if (isNewEntity) {
        ++view_.discoveredEntities;
    }
    ++view_.revision;
    return true;
}

const StationStore::ViewState& StationStore::viewState() const {
    return view_;
}

bool StationStore::sameCallsign(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return false;
    }

    while (*left != '\0' && *right != '\0') {
        char leftChar = *left;
        char rightChar = *right;
        if (leftChar >= 'a' && leftChar <= 'z') {
            leftChar = static_cast<char>(leftChar - 'a' + 'A');
        }
        if (rightChar >= 'a' && rightChar <= 'z') {
            rightChar = static_cast<char>(rightChar - 'a' + 'A');
        }
        if (leftChar != rightChar) {
            return false;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

bool StationStore::sameEntityName(const char* left, const char* right) {
    return left != nullptr && right != nullptr && std::strcmp(left, right) == 0;
}

bool StationStore::sameIdentity(
    const Station& station,
    const Aprs::ParsedFrame& frame) {

    if (station.type != frame.type) {
        return false;
    }
    if (frame.type == Aprs::EntityType::Station) {
        return sameCallsign(station.callsign, frame.source);
    }
    // Object and item names are case-sensitive and may change owner/source.
    return sameEntityName(station.entityName, frame.entityName);
}

void StationStore::removeAt(std::size_t index) {
    if (index >= view_.count) {
        return;
    }
    for (std::size_t current = index; current + 1 < view_.count; ++current) {
        view_.stations[current] = view_.stations[current + 1];
    }
    if (view_.count > 0) {
        --view_.count;
        view_.stations[view_.count] = Station{};
    }
}

}  // namespace Services
