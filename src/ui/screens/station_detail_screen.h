#pragma once

#include <cstdint>

#include "services/geo_utils.h"
#include "services/station_store.h"

namespace Ui {
namespace StationDetailScreen {

void create();
void scroll(int direction);
void update(
    const Services::StationStore::Station& station,
    const Services::PositionReference& reference,
    std::uint32_t now);

}  // namespace StationDetailScreen
}  // namespace Ui
