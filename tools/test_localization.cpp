#include <cassert>
#include <cstring>
#include <iostream>

#include "app/localization.h"

int main() {
    using App::Localization::isEnglish;
    using App::Localization::language;
    using App::Localization::revision;
    using App::Localization::setLanguage;
    using App::Localization::text;

    setLanguage(App::UiLanguage::Czech);
    const std::uint32_t czechRevision = revision();
    assert(language() == App::UiLanguage::Czech);
    assert(!isEnglish());
    assert(std::strcmp(text("Nastaveni", "Settings"), "Nastaveni") == 0);

    setLanguage(App::UiLanguage::English);
    assert(language() == App::UiLanguage::English);
    assert(isEnglish());
    assert(revision() == czechRevision + 1U);
    assert(std::strcmp(text("Nastaveni", "Settings"), "Settings") == 0);

    const std::uint32_t englishRevision = revision();
    setLanguage(App::UiLanguage::English);
    assert(revision() == englishRevision);

    setLanguage(App::UiLanguage::Czech);
    assert(revision() == englishRevision + 1U);

    std::cout << "localization tests passed\n";
    return 0;
}
