#pragma once

#include <aprs_codec.h>
#include <cstddef>
#include <cstdint>

namespace Services {

class StationStore {
public:
    static constexpr std::size_t MAX_STATIONS = 15;

    struct Station {
        bool used = false;
        bool hasPosition = false;
        Aprs::PositionFormat positionFormat = Aprs::PositionFormat::None;
        bool emergency = false;
        Aprs::EntityType type = Aprs::EntityType::Station;
        char callsign[Aprs::MAX_SOURCE_CALL_LENGTH + 1] = {};
        char entityName[Aprs::MAX_ENTITY_NAME_LENGTH + 1] = {};
        char symbol[3] = {'-', '-', '\0'};
        double latitude = 0.0;
        double longitude = 0.0;
        Aprs::TelemetryData telemetry;
        Aprs::PhgData phg;
        Aprs::FrequencyData frequency;
        float lastRssiDbm = 0.0F;
        float lastSnrDb = 0.0F;
        std::uint32_t lastHeardMs = 0;
        std::uint32_t heardCount = 0;
        char lastFrame[192] = {};
    };

    struct ViewState {
        Station stations[MAX_STATIONS] = {};
        std::size_t count = 0;
        // Monotonic event counter incremented only when a previously unknown
        // station, object or item is inserted. UI code can use it to create
        // a reliable new-entity notification even after the list is full.
        std::uint32_t discoveredEntities = 0;
        std::uint32_t revision = 0;
    };

    void clear();
    bool ingest(
        const Aprs::ParsedFrame& frame,
        float rssiDbm,
        float snrDb,
        std::uint32_t now,
        const char* lastFrame = nullptr);
    const ViewState& viewState() const;

private:
    static bool sameCallsign(const char* left, const char* right);
    static bool sameEntityName(const char* left, const char* right);
    static bool sameIdentity(const Station& station, const Aprs::ParsedFrame& frame);
    void removeAt(std::size_t index);

    ViewState view_;
};

}  // namespace Services
