#pragma once

#include <cstdint>

#include "app/app_types.h"

namespace App {
namespace Localization {

void setLanguage(UiLanguage language);
UiLanguage language();
bool isEnglish();
std::uint32_t revision();
const char* text(const char* czech, const char* english);

}  // namespace Localization
}  // namespace App
