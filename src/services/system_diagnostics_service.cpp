#include "services/system_diagnostics_service.h"

#include <Arduino.h>
#include <cstdio>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "app_config.h"
#include "app_log.h"

namespace Services {
namespace {

const char* resetReasonName(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON: return "power-on";
        case ESP_RST_EXT: return "external";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt WDT";
        case ESP_RST_TASK_WDT: return "task WDT";
        case ESP_RST_WDT: return "watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "SDIO";
        default: return "unknown";
    }
}

}  // namespace

bool SystemDiagnosticsService::begin(std::uint32_t now) {
    view_ = ViewState{};
    const esp_reset_reason_t reason = esp_reset_reason();
    view_.resetReasonCode = static_cast<std::uint8_t>(reason);
    std::snprintf(view_.resetReason, sizeof(view_.resetReason), "%s", resetReasonName(reason));
    sample(now, true);
    LOG_I(
        "DIAG",
        "Reset %s; internal heap %u B; PSRAM %u B; loop stack minimum %u B",
        view_.resetReason,
        static_cast<unsigned>(view_.freeInternalBytes),
        static_cast<unsigned>(view_.freePsramBytes),
        static_cast<unsigned>(view_.loopStackMinimumFreeBytes));
    return true;
}

void SystemDiagnosticsService::update(std::uint32_t now) {
    if (lastSampleAt_ != 0U &&
        now - lastSampleAt_ < AppConfig::SYSTEM_DIAGNOSTICS_SAMPLE_INTERVAL_MS) {
        return;
    }
    sample(now, false);
}

const SystemDiagnosticsService::ViewState& SystemDiagnosticsService::viewState() const {
    return view_;
}

void SystemDiagnosticsService::sample(std::uint32_t now, bool forceRevision) {
    const std::uint32_t freeInternal = static_cast<std::uint32_t>(
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    const std::uint32_t largestInternal = static_cast<std::uint32_t>(
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    const std::uint32_t minimumFreeInternal = static_cast<std::uint32_t>(
        heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
    const bool psramAvailable = psramFound();
    const std::uint32_t freePsram = psramAvailable
        ? static_cast<std::uint32_t>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM))
        : 0U;
    const std::uint32_t largestPsram = psramAvailable
        ? static_cast<std::uint32_t>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM))
        : 0U;
    // ESP-IDF's FreeRTOS port reports task stack high-water marks in bytes.
    const std::uint32_t stackMinimum = static_cast<std::uint32_t>(
        uxTaskGetStackHighWaterMark(nullptr));
    const std::uint32_t uptimeSeconds = now / 1000U;

    const bool changed = forceRevision ||
        freeInternal != view_.freeInternalBytes ||
        largestInternal != view_.largestInternalBlockBytes ||
        minimumFreeInternal != view_.minimumFreeInternalBytes ||
        psramAvailable != view_.psramAvailable ||
        freePsram != view_.freePsramBytes ||
        largestPsram != view_.largestPsramBlockBytes ||
        stackMinimum != view_.loopStackMinimumFreeBytes ||
        uptimeSeconds != view_.uptimeSeconds;

    view_.freeInternalBytes = freeInternal;
    view_.largestInternalBlockBytes = largestInternal;
    view_.minimumFreeInternalBytes = minimumFreeInternal;
    view_.psramAvailable = psramAvailable;
    view_.freePsramBytes = freePsram;
    view_.largestPsramBlockBytes = largestPsram;
    view_.loopStackMinimumFreeBytes = stackMinimum;
    view_.uptimeSeconds = uptimeSeconds;
    lastSampleAt_ = now;
    if (changed) {
        ++view_.revision;
    }
}

}  // namespace Services
