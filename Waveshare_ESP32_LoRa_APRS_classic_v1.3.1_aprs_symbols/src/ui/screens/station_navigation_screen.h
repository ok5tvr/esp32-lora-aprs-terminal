#pragma once

#include <cstdint>

#include "services/geo_utils.h"
#include "services/station_store.h"

namespace Ui {
namespace StationNavigationScreen {

void create();
void update(
    const Services::StationStore::Station& station,
    const Services::PositionReference& reference,
    std::uint32_t now);

}  // namespace StationNavigationScreen
}  // namespace Ui
