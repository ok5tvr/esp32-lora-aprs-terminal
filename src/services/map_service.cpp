#include "services/map_service.h"

#include <Arduino.h>
#include <algorithm>
#include <SD.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <esp_heap_caps.h>

#include "app_config.h"
#include "app_log.h"
#include "board_pins.h"
#include "drivers/sd_card_driver.h"

namespace Services {
namespace {

constexpr std::uint32_t TILE_FILE_SIZE =
    static_cast<std::uint32_t>(MapService::TILE_SIZE) *
    static_cast<std::uint32_t>(MapService::TILE_SIZE) * 2U;

std::uint16_t rgb565(std::uint8_t red, std::uint8_t green, std::uint8_t blue) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(red & 0xF8U) << 8U) |
        (static_cast<std::uint16_t>(green & 0xFCU) << 3U) |
        (static_cast<std::uint16_t>(blue) >> 3U));
}

}  // namespace

bool MapService::begin() {
    view_ = ViewState{};
    requestedZoom_ = AppConfig::MAP_DEFAULT_ZOOM;
    view_.zoom = requestedZoom_;
    view_.sdMounted = Drivers::SdCard::status().mounted;

    constexpr std::size_t bufferBytes =
        static_cast<std::size_t>(VIEW_WIDTH) * VIEW_HEIGHT * sizeof(std::uint16_t);
    buffer_ = static_cast<std::uint16_t*>(heap_caps_malloc(
        bufferBytes,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (buffer_ == nullptr) {
        setStatus("Nedostatek pameti pro mapovy framebuffer.");
        LOG_E("MAP", "Unable to allocate %u-byte map framebuffer",
              static_cast<unsigned>(bufferBytes));
        return false;
    }

    view_.pixels = buffer_;
    view_.initialized = true;
    clearBuffer();
    setStatus(view_.sdMounted
        ? "Mapa pripravena; cekam na otevreni obrazovky."
        : "SD karta neni dostupna; mapove dlazdice nelze nacist.");
    LOG_I(
        "MAP",
        "Framebuffer ready: %ux%u, %u bytes",
        static_cast<unsigned>(VIEW_WIDTH),
        static_cast<unsigned>(VIEW_HEIGHT),
        static_cast<unsigned>(bufferBytes));
    return true;
}

void MapService::update(
    std::uint32_t now,
    bool active,
    const PositionReference& reference) {

    (void)now;
    const bool sdMounted = Drivers::SdCard::status().mounted;
    if (view_.sdMounted != sdMounted) {
        view_.sdMounted = sdMounted;
        ++view_.viewRevision;
        if (!sdMounted) {
            closeCurrentTile();
            view_.loading = false;
            setStatus("SD karta neni dostupna; mapove dlazdice nelze nacist.");
        } else {
            forceRender_ = true;
        }
    }

    view_.active = active;
    if (!view_.initialized) {
        return;
    }

    if (!active) {
        if (wasActive_) {
            closeCurrentTile();
        }
        wasActive_ = false;
        return;
    }

    if (!wasActive_) {
        wasActive_ = true;
        forceRender_ = true;
    }

    if (!reference.valid) {
        view_.centerValid = false;
        view_.loading = false;
        closeCurrentTile();
        setStatus("Mapa ceka na GPS nebo vychozi polohu.");
        return;
    }

    if (forceRender_ || shouldRecenter(reference)) {
        requestRender(reference);
    } else if (reference.revision != lastReferenceRevision_) {
        lastReferenceRevision_ = reference.revision;
    }

    if (view_.loading && view_.sdMounted) {
        processOneChunk();
    }
}

void MapService::zoomIn() {
    if (requestedZoom_ < AppConfig::MAP_MAX_ZOOM) {
        ++requestedZoom_;
        forceRender_ = true;
    }
}

void MapService::zoomOut() {
    if (requestedZoom_ > AppConfig::MAP_MIN_ZOOM) {
        --requestedZoom_;
        forceRender_ = true;
    }
}

void MapService::recenter() {
    forceRender_ = true;
}

const MapService::ViewState& MapService::viewState() const {
    return view_;
}

MapProjection::ScreenPoint MapService::project(
    const ViewState& state,
    double latitude,
    double longitude) {

    if (!state.centerValid) {
        return {};
    }
    return MapProjection::projectToViewport(
        latitude,
        longitude,
        state.zoom,
        state.centerWorldX,
        state.centerWorldY,
        state.width,
        state.height);
}

bool MapService::shouldRecenter(const PositionReference& reference) const {
    if (!view_.centerValid || reference.revision == lastReferenceRevision_) {
        return false;
    }

    const MapProjection::ScreenPoint point = MapProjection::projectToViewport(
        reference.latitude,
        reference.longitude,
        view_.zoom,
        view_.centerWorldX,
        view_.centerWorldY,
        view_.width,
        view_.height);
    if (!point.valid) {
        return true;
    }

    const double centerX = static_cast<double>(view_.width) / 2.0;
    const double centerY = static_cast<double>(view_.height) / 2.0;
    return std::fabs(point.x - centerX) >= AppConfig::MAP_RECENTER_THRESHOLD_PIXELS ||
        std::fabs(point.y - centerY) >= AppConfig::MAP_RECENTER_THRESHOLD_PIXELS;
}

void MapService::requestRender(const PositionReference& reference) {
    closeCurrentTile();
    forceRender_ = false;
    lastReferenceRevision_ = reference.revision;
    view_.centerLatitude = reference.latitude;
    view_.centerLongitude = reference.longitude;
    view_.centerFromGps = reference.fromGps;
    view_.zoom = requestedZoom_;

    const MapProjection::WorldPixel center = MapProjection::toWorldPixel(
        reference.latitude,
        reference.longitude,
        view_.zoom);
    if (!center.valid) {
        view_.centerValid = false;
        view_.loading = false;
        setStatus("Neplatna poloha stredu mapy.");
        return;
    }

    view_.centerValid = true;
    view_.centerWorldX = center.x;
    view_.centerWorldY = center.y;
    view_.tileJobsTotal = 0;
    view_.tileJobsCompleted = 0;
    view_.missingTiles = 0;
    currentJob_ = 0;
    clearBuffer();
    buildJobs();
    view_.loading = view_.tileJobsTotal > 0U && view_.sdMounted;
    ++view_.viewRevision;

    if (!view_.sdMounted) {
        setStatus("SD karta neni dostupna; zobrazuji prazdnou mapu.");
    } else if (view_.tileJobsTotal == 0U) {
        setStatus("Pro aktualni vyrez nebyly vytvoreny mapove ulohy.");
    } else {
        setStatus("Nacitam offline mapu z SD karty...");
    }
}

void MapService::buildJobs() {
    const double topLeftX = view_.centerWorldX - static_cast<double>(VIEW_WIDTH) / 2.0;
    const double topLeftY = view_.centerWorldY - static_cast<double>(VIEW_HEIGHT) / 2.0;
    const double bottomRightX = topLeftX + static_cast<double>(VIEW_WIDTH) - 1.0;
    const double bottomRightY = topLeftY + static_cast<double>(VIEW_HEIGHT) - 1.0;

    const std::int32_t firstTileX = static_cast<std::int32_t>(std::floor(topLeftX / TILE_SIZE));
    const std::int32_t lastTileX = static_cast<std::int32_t>(std::floor(bottomRightX / TILE_SIZE));
    const std::int32_t firstTileY = static_cast<std::int32_t>(std::floor(topLeftY / TILE_SIZE));
    const std::int32_t lastTileY = static_cast<std::int32_t>(std::floor(bottomRightY / TILE_SIZE));

    for (std::int32_t tileY = firstTileY; tileY <= lastTileY; ++tileY) {
        for (std::int32_t tileX = firstTileX; tileX <= lastTileX; ++tileX) {
            if (view_.tileJobsTotal >= MAX_TILE_JOBS) {
                return;
            }

            const double tileOriginX = static_cast<double>(tileX) * TILE_SIZE;
            const double tileOriginY = static_cast<double>(tileY) * TILE_SIZE;
            std::int32_t destinationX = static_cast<std::int32_t>(std::floor(tileOriginX - topLeftX));
            std::int32_t destinationY = static_cast<std::int32_t>(std::floor(tileOriginY - topLeftY));
            std::int32_t sourceX = 0;
            std::int32_t sourceY = 0;
            if (destinationX < 0) {
                sourceX = -destinationX;
                destinationX = 0;
            }
            if (destinationY < 0) {
                sourceY = -destinationY;
                destinationY = 0;
            }

            const std::int32_t copyWidth = std::min<std::int32_t>(
                TILE_SIZE - sourceX,
                VIEW_WIDTH - destinationX);
            const std::int32_t copyHeight = std::min<std::int32_t>(
                TILE_SIZE - sourceY,
                VIEW_HEIGHT - destinationY);
            if (copyWidth <= 0 || copyHeight <= 0) {
                continue;
            }

            TileJob& job = jobs_[view_.tileJobsTotal++];
            job = TileJob{};
            job.logicalTileX = tileX;
            job.tileY = tileY;
            job.destinationX = static_cast<std::int16_t>(destinationX);
            job.destinationY = static_cast<std::int16_t>(destinationY);
            job.sourceX = static_cast<std::uint16_t>(sourceX);
            job.sourceY = static_cast<std::uint16_t>(sourceY);
            job.width = static_cast<std::uint16_t>(copyWidth);
            job.height = static_cast<std::uint16_t>(copyHeight);
        }
    }
}

void MapService::processOneChunk() {
    if (currentJob_ >= view_.tileJobsTotal) {
        completeRender();
        return;
    }

    TileJob& job = jobs_[currentJob_];
    if (!job.opened && !openCurrentTile(job)) {
        fillMissing(job);
        finishCurrentTile(true);
        return;
    }

    const std::uint16_t remaining = static_cast<std::uint16_t>(job.height - job.nextRow);
    const std::uint16_t rowCount = std::min<std::uint16_t>(
        remaining,
        AppConfig::MAP_TILE_ROWS_PER_UPDATE);
    bool readFailed = false;

    digitalWrite(BoardPins::LCD_CS, HIGH);
    for (std::uint16_t row = 0; row < rowCount; ++row) {
        const std::uint16_t sourceRow = static_cast<std::uint16_t>(
            job.sourceY + job.nextRow + row);
        const std::uint32_t fileOffset =
            (static_cast<std::uint32_t>(sourceRow) * TILE_SIZE + job.sourceX) * 2U;
        std::uint16_t* destination = buffer_ +
            static_cast<std::size_t>(job.destinationY + job.nextRow + row) * VIEW_WIDTH +
            job.destinationX;
        const std::size_t requestedBytes = static_cast<std::size_t>(job.width) * 2U;
        const std::size_t bytesRead = currentTile_.seek(fileOffset)
            ? currentTile_.read(reinterpret_cast<std::uint8_t*>(destination), requestedBytes)
            : 0U;
        if (bytesRead != requestedBytes) {
            readFailed = true;
            fillMissingRows(job, static_cast<std::uint16_t>(job.nextRow + row), 1U);
        }
    }

    job.nextRow = static_cast<std::uint16_t>(job.nextRow + rowCount);
    ++view_.bufferRevision;
    if (job.nextRow >= job.height) {
        finishCurrentTile(readFailed);
    }
}

bool MapService::openCurrentTile(TileJob& job) {
    closeCurrentTile();
    const std::int32_t tileCount = 1L << view_.zoom;
    if (job.tileY < 0 || job.tileY >= tileCount) {
        return false;
    }

    const std::int32_t tileX = wrapTileX(job.logicalTileX, view_.zoom);
    char path[80];
    std::snprintf(
        path,
        sizeof(path),
        "%s/%u/%ld/%ld.rgb",
        AppConfig::MAP_DIRECTORY,
        static_cast<unsigned>(view_.zoom),
        static_cast<long>(tileX),
        static_cast<long>(job.tileY));

    digitalWrite(BoardPins::LCD_CS, HIGH);
    currentTile_ = SD.open(path, FILE_READ);
    if (!currentTile_ || currentTile_.isDirectory() || currentTile_.size() < TILE_FILE_SIZE) {
        closeCurrentTile();
        return false;
    }
    job.opened = true;
    return true;
}

void MapService::finishCurrentTile(bool missing) {
    closeCurrentTile();
    if (missing) {
        ++view_.missingTiles;
    }
    ++view_.tileJobsCompleted;
    ++currentJob_;
    ++view_.viewRevision;
    if (currentJob_ >= view_.tileJobsTotal) {
        completeRender();
    }
}

void MapService::completeRender() {
    view_.loading = false;
    if (view_.missingTiles == 0U) {
        setStatus("Offline mapa nactena.");
    } else {
        char text[112];
        std::snprintf(
            text,
            sizeof(text),
            "Mapa nactena; chybi %u dlazdic na SD karte.",
            static_cast<unsigned>(view_.missingTiles));
        setStatus(text);
    }
}

void MapService::closeCurrentTile() {
    if (currentTile_) {
        currentTile_.close();
    }
    if (currentJob_ < view_.tileJobsTotal) {
        jobs_[currentJob_].opened = false;
    }
}

void MapService::clearBuffer() {
    if (buffer_ == nullptr) {
        return;
    }
    const std::uint16_t dark = rgb565(18, 28, 43);
    const std::uint16_t light = rgb565(25, 39, 59);
    for (std::uint16_t y = 0; y < VIEW_HEIGHT; ++y) {
        for (std::uint16_t x = 0; x < VIEW_WIDTH; ++x) {
            const bool alternate = ((x / 32U) + (y / 32U)) % 2U != 0U;
            buffer_[static_cast<std::size_t>(y) * VIEW_WIDTH + x] = alternate ? light : dark;
        }
    }
    ++view_.bufferRevision;
}

void MapService::fillMissing(const TileJob& job) {
    fillMissingRows(job, 0U, job.height);
    ++view_.bufferRevision;
}

void MapService::fillMissingRows(
    const TileJob& job,
    std::uint16_t startRow,
    std::uint16_t rowCount) {

    if (buffer_ == nullptr) {
        return;
    }
    const std::uint16_t dark = rgb565(42, 48, 61);
    const std::uint16_t light = rgb565(57, 64, 78);
    const std::uint16_t endRow = std::min<std::uint16_t>(
        job.height,
        static_cast<std::uint16_t>(startRow + rowCount));
    for (std::uint16_t row = startRow; row < endRow; ++row) {
        const std::uint16_t y = static_cast<std::uint16_t>(job.destinationY + row);
        for (std::uint16_t column = 0; column < job.width; ++column) {
            const std::uint16_t x = static_cast<std::uint16_t>(job.destinationX + column);
            const bool alternate = (((job.sourceX + column) / 16U) +
                                    ((job.sourceY + row) / 16U)) % 2U != 0U;
            buffer_[static_cast<std::size_t>(y) * VIEW_WIDTH + x] = alternate ? light : dark;
        }
    }
}

void MapService::setStatus(const char* text) {
    const char* safeText = text != nullptr ? text : "";
    if (std::strncmp(view_.statusText, safeText, sizeof(view_.statusText)) != 0) {
        std::snprintf(view_.statusText, sizeof(view_.statusText), "%s", safeText);
        ++view_.viewRevision;
    }
}

std::int32_t MapService::wrapTileX(std::int32_t tileX, std::uint8_t zoom) {
    const std::int32_t count = 1L << zoom;
    tileX %= count;
    if (tileX < 0) {
        tileX += count;
    }
    return tileX;
}

}  // namespace Services
