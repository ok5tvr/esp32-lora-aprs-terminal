#pragma once

#include <FS.h>
#include <cstddef>
#include <cstdint>

#include "services/geo_utils.h"
#include "services/map_projection.h"

namespace Services {

class MapService {
public:
    static constexpr std::uint16_t VIEW_WIDTH = 480;
    static constexpr std::uint16_t VIEW_HEIGHT = 202;
    static constexpr std::uint16_t TILE_SIZE = MapProjection::TILE_SIZE;
    static constexpr std::size_t MAX_TILE_JOBS = 9;

    struct ViewState {
        bool initialized = false;
        bool sdMounted = false;
        bool active = false;
        bool centerValid = false;
        bool centerFromGps = false;
        bool followReference = true;
        bool loading = false;
        std::uint16_t* pixels = nullptr;
        std::uint16_t width = VIEW_WIDTH;
        std::uint16_t height = VIEW_HEIGHT;
        std::uint8_t zoom = 13;
        double centerLatitude = 0.0;
        double centerLongitude = 0.0;
        double centerWorldX = 0.0;
        double centerWorldY = 0.0;
        std::uint8_t tileJobsTotal = 0;
        std::uint8_t tileJobsCompleted = 0;
        std::uint8_t missingTiles = 0;
        std::uint32_t bufferRevision = 0;
        std::uint32_t viewRevision = 0;
        char statusText[112] = "--";
    };

    bool begin();
    void update(
        std::uint32_t now,
        bool active,
        const PositionReference& reference);
    void zoomIn();
    void zoomOut();
    void recenter();
    void panByPixels(std::int16_t deltaX, std::int16_t deltaY);
    const ViewState& viewState() const;

    static MapProjection::ScreenPoint project(
        const ViewState& state,
        double latitude,
        double longitude);

private:
    struct TileJob {
        std::int32_t logicalTileX = 0;
        std::int32_t tileY = 0;
        std::int16_t destinationX = 0;
        std::int16_t destinationY = 0;
        std::uint16_t sourceX = 0;
        std::uint16_t sourceY = 0;
        std::uint16_t width = 0;
        std::uint16_t height = 0;
        std::uint16_t nextRow = 0;
        bool opened = false;
    };

    bool shouldRecenter(const PositionReference& reference) const;
    void requestRender(const PositionReference& reference);
    void buildJobs();
    void processOneChunk();
    bool openCurrentTile(TileJob& job);
    void finishCurrentTile(bool missing);
    void completeRender();
    void closeCurrentTile();
    void clearBuffer();
    void fillMissing(const TileJob& job);
    void fillMissingRows(const TileJob& job, std::uint16_t startRow, std::uint16_t rowCount);
    void setStatus(const char* text);
    static std::int32_t wrapTileX(std::int32_t tileX, std::uint8_t zoom);

    ViewState view_;
    std::uint16_t* buffer_ = nullptr;
    TileJob jobs_[MAX_TILE_JOBS] = {};
    std::uint8_t currentJob_ = 0;
    File currentTile_;
    std::uint8_t requestedZoom_ = 13;
    PositionReference manualReference_;
    std::uint32_t manualReferenceRevision_ = 0;
    std::uint32_t lastReferenceRevision_ = 0;
    bool followReference_ = true;
    bool forceRender_ = true;
    bool wasActive_ = false;
};

}  // namespace Services
