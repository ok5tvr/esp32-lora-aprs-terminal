#pragma once
#include <cstdint>
#include "services/geo_utils.h"
#include "services/weather_store.h"
namespace Ui { namespace WeatherDetailScreen {
void create();
void update(const Services::WeatherStore::WeatherStation& station, const Services::PositionReference& reference, std::uint32_t now);
} }
