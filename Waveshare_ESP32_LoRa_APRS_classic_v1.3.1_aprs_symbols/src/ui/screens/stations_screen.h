#pragma once

#include <cstddef>

#include "services/geo_utils.h"
#include "services/station_store.h"

namespace Ui {
namespace StationsScreen {

void create();
void update(
    const Services::StationStore::ViewState& state,
    const Services::PositionReference& reference);
void moveSelection(int direction);
std::size_t selectedIndex();

}  // namespace StationsScreen
}  // namespace Ui
