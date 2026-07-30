#pragma once

#include "services/gps_service.h"

namespace Ui {
namespace GpsScreen {

void create();
void update(const Services::GpsService::ViewState& state);

}  // namespace GpsScreen
}  // namespace Ui
