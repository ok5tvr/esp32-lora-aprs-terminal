#pragma once

#include "services/geo_utils.h"
#include "services/map_service.h"
#include "services/station_store.h"
#include "services/trail_service.h"

namespace Ui::MapScreen {

void create();
void update(
    const Services::MapService::ViewState& mapState,
    const Services::StationStore::ViewState& stationState,
    const Services::TrailService::ViewState& trailState,
    const Services::PositionReference& reference);

}  // namespace Ui::MapScreen
