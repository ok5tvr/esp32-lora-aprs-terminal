#pragma once

#include "services/geo_utils.h"
#include "services/weather_store.h"

namespace Ui {
namespace WeatherScreen {

void create();
void update(
    const Services::WeatherStore::ViewState& state,
    const Services::PositionReference& reference);
void scroll(int direction);

}  // namespace WeatherScreen
}  // namespace Ui
