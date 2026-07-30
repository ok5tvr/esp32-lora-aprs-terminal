#include "ui/screens/map_screen.h"

#include <cmath>
#include <cstdio>
#include <lvgl.h>

#include "app_config.h"
#include "ui/aprs_icons.h"
#include "ui/ui_components.h"

namespace Ui::MapScreen {
namespace {

static_assert(sizeof(lv_color_t) == sizeof(std::uint16_t),
              "Offline map canvas requires LV_COLOR_DEPTH 16");

lv_obj_t* canvas = nullptr;
lv_obj_t* overlay = nullptr;
lv_obj_t* statusLabel = nullptr;
lv_obj_t* trailLine = nullptr;
lv_point_t trailPoints[AppConfig::MAP_RECENT_TRAIL_POINTS] = {};
lv_obj_t* stationMarkers[Services::StationStore::MAX_STATIONS] = {};
lv_obj_t* ownMarker = nullptr;
std::uint16_t* attachedBuffer = nullptr;
std::uint32_t renderedBufferRevision = 0xFFFFFFFFUL;
std::uint32_t renderedMapRevision = 0xFFFFFFFFUL;
std::uint32_t renderedStationRevision = 0xFFFFFFFFUL;
std::uint32_t renderedTrailRevision = 0xFFFFFFFFUL;
std::uint32_t renderedReferenceRevision = 0xFFFFFFFFUL;

bool visibleWithMargin(const Services::MapProjection::ScreenPoint& point, int margin) {
    return point.valid &&
        point.x >= -margin && point.x < Services::MapService::VIEW_WIDTH + margin &&
        point.y >= -margin && point.y < Services::MapService::VIEW_HEIGHT + margin;
}

void clearStationMarkers() {
    for (lv_obj_t*& marker : stationMarkers) {
        if (marker != nullptr) {
            lv_obj_del(marker);
            marker = nullptr;
        }
    }
}

void updateTrail(
    const Services::MapService::ViewState& mapState,
    const Services::TrailService::ViewState& trailState) {

    if (trailLine == nullptr) {
        return;
    }

    std::uint16_t count = 0;
    for (std::uint8_t index = 0;
         index < trailState.recentPointCount && count < AppConfig::MAP_RECENT_TRAIL_POINTS;
         ++index) {
        const auto point = Services::MapService::project(
            mapState,
            trailState.recentPoints[index].latitude,
            trailState.recentPoints[index].longitude);
        if (!visibleWithMargin(point, 0)) {
            continue;
        }
        trailPoints[count].x = static_cast<lv_coord_t>(std::lround(point.x));
        trailPoints[count].y = static_cast<lv_coord_t>(std::lround(point.y));
        ++count;
    }

    if (count >= 2U) {
        lv_line_set_points(trailLine, trailPoints, count);
        lv_obj_clear_flag(trailLine, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(trailLine, LV_OBJ_FLAG_HIDDEN);
    }
}

void updateStations(
    const Services::MapService::ViewState& mapState,
    const Services::StationStore::ViewState& stationState) {

    clearStationMarkers();
    if (overlay == nullptr || !mapState.centerValid) {
        return;
    }

    for (std::size_t index = 0; index < stationState.count; ++index) {
        const auto& station = stationState.stations[index];
        if (!station.hasPosition) {
            continue;
        }
        const auto point = Services::MapService::project(
            mapState,
            station.latitude,
            station.longitude);
        if (!visibleWithMargin(point, 20)) {
            continue;
        }

        lv_obj_t* marker = AprsIcons::create(
            overlay,
            station.symbol[0],
            station.symbol[1],
            true);
        stationMarkers[index] = marker;
        lv_obj_set_pos(
            marker,
            static_cast<lv_coord_t>(std::lround(point.x)) - 16,
            static_cast<lv_coord_t>(std::lround(point.y)) - 16);
        if (station.emergency) {
            lv_obj_set_style_border_color(marker, lv_color_hex(0xFF3B4D), 0);
            lv_obj_set_style_border_width(marker, 3, 0);
        }
    }
}

void updateOwnPosition(
    const Services::MapService::ViewState& mapState,
    const Services::PositionReference& reference) {

    if (ownMarker == nullptr) {
        return;
    }
    if (!reference.valid || !mapState.centerValid) {
        lv_obj_add_flag(ownMarker, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    const auto point = Services::MapService::project(
        mapState,
        reference.latitude,
        reference.longitude);
    if (!visibleWithMargin(point, 8)) {
        lv_obj_add_flag(ownMarker, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_set_pos(
        ownMarker,
        static_cast<lv_coord_t>(std::lround(point.x)) - 7,
        static_cast<lv_coord_t>(std::lround(point.y)) - 7);
    lv_obj_clear_flag(ownMarker, LV_OBJ_FLAG_HIDDEN);
}

void updateStatus(const Services::MapService::ViewState& state) {
    if (statusLabel == nullptr) {
        return;
    }

    if (!state.initialized) {
        lv_label_set_text(statusLabel, "Mapa: nedostatek pameti");
        lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xFF6B6B), 0);
        return;
    }

    if (!state.sdMounted) {
        lv_label_set_text_fmt(statusLabel, "Z%u | SD neni dostupna", state.zoom);
        lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xFFB454), 0);
        return;
    }

    if (state.loading) {
        lv_label_set_text_fmt(
            statusLabel,
            "Z%u | nacitam %u/%u | chybi %u",
            state.zoom,
            static_cast<unsigned>(state.tileJobsCompleted),
            static_cast<unsigned>(state.tileJobsTotal),
            static_cast<unsigned>(state.missingTiles));
        lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x56C7FF), 0);
    } else {
        lv_label_set_text_fmt(
            statusLabel,
            "Z%u | %s | chybi %u | UP/DOWN zoom, OK stred",
            state.zoom,
            state.centerFromGps ? "GPS" : "DEF",
            static_cast<unsigned>(state.missingTiles));
        lv_obj_set_style_text_color(
            statusLabel,
            state.missingTiles == 0U ? lv_color_hex(0xF4F7FF) : lv_color_hex(0xFFB454),
            0);
    }
}

}  // namespace

void create() {
    resetScreen();
    createHeader("Offline mapa");

    canvas = lv_canvas_create(lv_scr_act());
    lv_obj_set_size(canvas, Services::MapService::VIEW_WIDTH, Services::MapService::VIEW_HEIGHT);
    lv_obj_align(canvas, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(canvas, lv_color_hex(0x121C2B), 0);
    lv_obj_set_style_bg_opa(canvas, LV_OPA_COVER, 0);

    overlay = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, Services::MapService::VIEW_WIDTH, Services::MapService::VIEW_HEIGHT);
    lv_obj_align(overlay, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

    trailLine = lv_line_create(overlay);
    lv_obj_set_size(trailLine, Services::MapService::VIEW_WIDTH, Services::MapService::VIEW_HEIGHT);
    lv_obj_set_pos(trailLine, 0, 0);
    lv_obj_set_style_line_color(trailLine, lv_color_hex(0xFFB454), 0);
    lv_obj_set_style_line_width(trailLine, 3, 0);
    lv_obj_set_style_line_rounded(trailLine, true, 0);
    lv_obj_add_flag(trailLine, LV_OBJ_FLAG_HIDDEN);

    ownMarker = lv_obj_create(overlay);
    lv_obj_remove_style_all(ownMarker);
    lv_obj_set_size(ownMarker, 14, 14);
    lv_obj_set_style_radius(ownMarker, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ownMarker, lv_color_hex(0x00D4FF), 0);
    lv_obj_set_style_bg_opa(ownMarker, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ownMarker, lv_color_white(), 0);
    lv_obj_set_style_border_width(ownMarker, 2, 0);
    lv_obj_clear_flag(ownMarker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ownMarker, LV_OBJ_FLAG_CLICKABLE);

    statusLabel = lv_label_create(overlay);
    lv_obj_set_width(statusLabel, 464);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_set_style_bg_color(statusLabel, lv_color_hex(0x0B1424), 0);
    lv_obj_set_style_bg_opa(statusLabel, LV_OPA_70, 0);
    lv_obj_set_style_radius(statusLabel, 5, 0);
    lv_obj_set_style_pad_left(statusLabel, 5, 0);
    lv_obj_set_style_pad_right(statusLabel, 5, 0);
    lv_obj_set_style_pad_top(statusLabel, 2, 0);
    lv_obj_set_style_pad_bottom(statusLabel, 2, 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 8, 6);

    attachedBuffer = nullptr;
    renderedBufferRevision = 0xFFFFFFFFUL;
    renderedMapRevision = 0xFFFFFFFFUL;
    renderedStationRevision = 0xFFFFFFFFUL;
    renderedTrailRevision = 0xFFFFFFFFUL;
    renderedReferenceRevision = 0xFFFFFFFFUL;
    for (lv_obj_t*& marker : stationMarkers) {
        marker = nullptr;
    }
}

void update(
    const Services::MapService::ViewState& mapState,
    const Services::StationStore::ViewState& stationState,
    const Services::TrailService::ViewState& trailState,
    const Services::PositionReference& reference) {

    if (canvas == nullptr || overlay == nullptr) {
        return;
    }

    if (mapState.pixels != nullptr && attachedBuffer != mapState.pixels) {
        attachedBuffer = mapState.pixels;
        lv_canvas_set_buffer(
            canvas,
            attachedBuffer,
            mapState.width,
            mapState.height,
            LV_IMG_CF_TRUE_COLOR);
        renderedBufferRevision = 0xFFFFFFFFUL;
    }

    if (attachedBuffer != nullptr && renderedBufferRevision != mapState.bufferRevision) {
        renderedBufferRevision = mapState.bufferRevision;
        lv_obj_invalidate(canvas);
    }

    updateStatus(mapState);

    const bool mapChanged = renderedMapRevision != mapState.viewRevision;
    if (mapChanged || renderedTrailRevision != trailState.recentPointRevision) {
        renderedMapRevision = mapState.viewRevision;
        renderedTrailRevision = trailState.recentPointRevision;
        updateTrail(mapState, trailState);
    }

    if (mapChanged || renderedStationRevision != stationState.revision) {
        renderedStationRevision = stationState.revision;
        updateStations(mapState, stationState);
    }

    if (mapChanged || renderedReferenceRevision != reference.revision) {
        renderedReferenceRevision = reference.revision;
        updateOwnPosition(mapState, reference);
    }

    if (statusLabel != nullptr) {
        lv_obj_move_foreground(statusLabel);
    }
    if (ownMarker != nullptr) {
        lv_obj_move_foreground(ownMarker);
    }
}

}  // namespace Ui::MapScreen
