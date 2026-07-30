#pragma once

#include "services/power_service.h"

namespace Ui {
namespace PowerScreen {

void create();
void update(const Services::PowerService::ViewState& state);

}  // namespace PowerScreen
}  // namespace Ui
