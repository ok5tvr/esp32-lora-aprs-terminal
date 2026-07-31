#include "app/localization.h"

namespace App {
namespace Localization {
namespace {
UiLanguage activeLanguage = UiLanguage::Czech;
std::uint32_t activeRevision = 0;
}

void setLanguage(UiLanguage language) {
    if (activeLanguage != language) {
        activeLanguage = language;
        ++activeRevision;
    }
}

UiLanguage language() {
    return activeLanguage;
}

bool isEnglish() {
    return activeLanguage == UiLanguage::English;
}

std::uint32_t revision() {
    return activeRevision;
}

const char* text(const char* czech, const char* english) {
    return isEnglish() ? english : czech;
}

}  // namespace Localization
}  // namespace App
