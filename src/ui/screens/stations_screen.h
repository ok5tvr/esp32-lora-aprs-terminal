#pragma once

#include "services/geo_utils.h"
#include "services/station_store.h"

namespace Ui {
namespace StationsScreen {

void create();
void update(
    const Services::StationStore::ViewState& state,
    const Services::PositionReference& reference);
void scroll(int direction);

}  // namespace StationsScreen
}  // namespace Ui
