#pragma once

#include <cstddef>

#include "services/geo_utils.h"
#include "services/weather_store.h"

namespace Ui {
namespace WeatherScreen {

void create();
void update(
    const Services::WeatherStore::ViewState& state,
    const Services::PositionReference& reference);
void moveSelection(int direction);
std::size_t selectedIndex();

}  // namespace WeatherScreen
}  // namespace Ui
