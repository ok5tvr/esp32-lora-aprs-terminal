#pragma once

#include <cstddef>
#include <cstdint>

namespace Services {

class SystemDiagnosticsService {
public:
    struct ViewState {
        bool psramAvailable = false;
        std::uint32_t freeInternalBytes = 0;
        std::uint32_t largestInternalBlockBytes = 0;
        std::uint32_t minimumFreeInternalBytes = 0;
        std::uint32_t freePsramBytes = 0;
        std::uint32_t largestPsramBlockBytes = 0;
        std::uint32_t loopStackMinimumFreeBytes = 0;
        std::uint32_t uptimeSeconds = 0;
        std::uint32_t revision = 0;
        std::uint8_t resetReasonCode = 0;
        char resetReason[32] = "unknown";
    };

    bool begin(std::uint32_t now);
    void update(std::uint32_t now);
    const ViewState& viewState() const;

private:
    void sample(std::uint32_t now, bool forceRevision);

    ViewState view_;
    std::uint32_t lastSampleAt_ = 0;
};

}  // namespace Services
