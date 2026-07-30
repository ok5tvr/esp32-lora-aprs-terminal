#pragma once

#include "services/radio_service.h"

namespace Ui {
namespace LoRaScreen {

void create();
void update(const Services::RadioService::ViewState& state);
void setMessage(const char* text);

}  // namespace LoRaScreen
}  // namespace Ui
