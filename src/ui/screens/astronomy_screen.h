#pragma once

#include "services/astronomy_service.h"
#include "services/time_service.h"

namespace Ui {
namespace AstronomyScreen {

void create();
void update(
    const Services::AstronomyService::ViewState& state,
    const Services::TimeService::ViewState& timeState);

}  // namespace AstronomyScreen
}  // namespace Ui
