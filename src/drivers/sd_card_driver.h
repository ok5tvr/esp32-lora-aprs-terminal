#pragma once

#include <cstdint>

namespace Drivers {
namespace SdCard {

struct Status {
    bool mounted = false;
    std::uint8_t cardType = 0;
    std::uint64_t cardSizeBytes = 0;
    std::uint64_t totalBytes = 0;
    std::uint64_t usedBytes = 0;
};

bool begin();
const Status& status();
void refreshUsage();

}  // namespace SdCard
}  // namespace Drivers
