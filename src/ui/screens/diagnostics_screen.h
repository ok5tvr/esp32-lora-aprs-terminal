#pragma once

#include <cstdint>

#include "services/ota_state.h"
#include "services/radio_service.h"
#include "services/station_store.h"
#include "services/system_diagnostics_service.h"

namespace Ui {
namespace DiagnosticsScreen {

void create();
void update(
    const Services::RadioService::ViewState& radioState,
    const Services::SystemDiagnosticsService::ViewState& systemState,
    const Services::OtaViewState& otaState,
    const Services::StationStore::ViewState& stationState,
    std::uint32_t now);

}  // namespace DiagnosticsScreen
}  // namespace Ui
