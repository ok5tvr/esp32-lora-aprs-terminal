#pragma once

#include <cstdint>

#include "services/radio_service.h"

namespace Ui {
namespace DiagnosticsScreen {

void create();
void update(const Services::RadioService::ViewState& state, std::uint32_t now);

}  // namespace DiagnosticsScreen
}  // namespace Ui
