#include "ui/screens/diagnostics_screen.h"

#include <cmath>
#include <cstdio>
#include <lvgl.h>

#include "app_config.h"
#include "app/localization.h"
#include "ui/ui_components.h"

namespace Ui {
namespace DiagnosticsScreen {
namespace {

lv_obj_t* latestLabel = nullptr;
lv_obj_t* statsLabel = nullptr;
lv_obj_t* scheduleLabel = nullptr;
lv_obj_t* chart = nullptr;
lv_chart_series_t* averageSeries = nullptr;
lv_chart_series_t* peakSeries = nullptr;
lv_obj_t* memoryLabel = nullptr;
lv_obj_t* systemLabel = nullptr;
lv_obj_t* stateLabel = nullptr;
std::uint32_t renderedRadioRevision = 0xFFFFFFFFUL;

std::uint32_t secondsUntil(std::uint32_t now, std::uint32_t target) {
    const std::int32_t remaining = static_cast<std::int32_t>(target - now);
    if (remaining <= 0) {
        return 0;
    }
    return (static_cast<std::uint32_t>(remaining) + 999U) / 1000U;
}

lv_coord_t chartValue(float value) {
    if (!std::isfinite(value)) {
        return LV_CHART_POINT_NONE;
    }
    if (value < -165.0F) {
        value = -165.0F;
    } else if (value > -40.0F) {
        value = -40.0F;
    }
    return static_cast<lv_coord_t>(std::lround(value));
}

unsigned kib(std::uint32_t bytes) {
    return static_cast<unsigned>((bytes + 512U) / 1024U);
}

void formatUptime(std::uint32_t seconds, char* output, std::size_t capacity) {
    const std::uint32_t days = seconds / 86400U;
    seconds %= 86400U;
    const std::uint32_t hours = seconds / 3600U;
    const std::uint32_t minutes = (seconds % 3600U) / 60U;
    if (days > 0U) {
        std::snprintf(
            output, capacity, "%lud %02lu:%02lu",
            static_cast<unsigned long>(days),
            static_cast<unsigned long>(hours),
            static_cast<unsigned long>(minutes));
    } else {
        std::snprintf(
            output, capacity, "%02lu:%02lu:%02lu",
            static_cast<unsigned long>(hours),
            static_cast<unsigned long>(minutes),
            static_cast<unsigned long>(seconds % 60U));
    }
}

void refreshChart(const Services::RadioService::ViewState& state) {
    if (chart == nullptr || averageSeries == nullptr || peakSeries == nullptr) {
        return;
    }

    for (std::size_t index = 0; index < AppConfig::RADIO_NOISE_HISTORY_LENGTH; ++index) {
        const bool valid = index < state.noiseHistoryCount;
        lv_chart_set_value_by_id(
            chart,
            averageSeries,
            static_cast<std::uint16_t>(index),
            valid ? chartValue(state.noiseHistoryDbm[index]) : LV_CHART_POINT_NONE);
        lv_chart_set_value_by_id(
            chart,
            peakSeries,
            static_cast<std::uint16_t>(index),
            valid ? chartValue(state.noisePeakHistoryDbm[index]) : LV_CHART_POINT_NONE);
    }
    lv_chart_refresh(chart);
}

void styleCompactLabel(lv_obj_t* label, lv_color_t color, int y) {
    lv_obj_set_width(label, 434);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, y);
}

}  // namespace

void create() {
    resetScreen();
    createHeader(App::Localization::text("Diagnostika", "Diagnostics"));

    lv_obj_t* summary = lv_obj_create(lv_scr_act());
    lv_obj_set_size(summary, 452, 57);
    lv_obj_align(summary, LV_ALIGN_TOP_MID, 0, 52);
    lv_obj_set_style_bg_color(summary, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(summary, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(summary, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(summary, 1, 0);
    lv_obj_set_style_radius(summary, 9, 0);
    lv_obj_set_style_pad_all(summary, 6, 0);
    lv_obj_clear_flag(summary, LV_OBJ_FLAG_SCROLLABLE);

    latestLabel = lv_label_create(summary);
    lv_obj_set_width(latestLabel, 438);
    lv_label_set_long_mode(latestLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(latestLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(latestLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(latestLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    statsLabel = lv_label_create(summary);
    styleCompactLabel(statsLabel, lv_color_hex(0xBDCAE0), 19);

    scheduleLabel = lv_label_create(summary);
    styleCompactLabel(scheduleLabel, lv_color_hex(0x92A7C7), 36);

    chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart, 452, 76);
    lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 113);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, AppConfig::RADIO_NOISE_HISTORY_LENGTH);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -165, -40);
    lv_chart_set_div_line_count(chart, 4, 4);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x101D31), 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(chart, 1, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x263A59), LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
    lv_obj_set_style_size(chart, 3, LV_PART_INDICATOR);

    averageSeries = lv_chart_add_series(
        chart, lv_color_hex(0x56C7FF), LV_CHART_AXIS_PRIMARY_Y);
    peakSeries = lv_chart_add_series(
        chart, lv_color_hex(0xFFB454), LV_CHART_AXIS_PRIMARY_Y);

    lv_obj_t* system = lv_obj_create(lv_scr_act());
    lv_obj_set_size(system, 452, 56);
    lv_obj_align(system, LV_ALIGN_TOP_MID, 0, 193);
    lv_obj_set_style_bg_color(system, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(system, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(system, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(system, 1, 0);
    lv_obj_set_style_radius(system, 9, 0);
    lv_obj_set_style_pad_all(system, 5, 0);
    lv_obj_clear_flag(system, LV_OBJ_FLAG_SCROLLABLE);

    memoryLabel = lv_label_create(system);
    styleCompactLabel(memoryLabel, lv_color_hex(0xBDCAE0), 0);
    systemLabel = lv_label_create(system);
    styleCompactLabel(systemLabel, lv_color_hex(0xBDCAE0), 17);
    stateLabel = lv_label_create(system);
    styleCompactLabel(stateLabel, lv_color_hex(0x92A7C7), 34);

    renderedRadioRevision = 0xFFFFFFFFUL;
}

void update(
    const Services::RadioService::ViewState& radioState,
    const Services::SystemDiagnosticsService::ViewState& systemState,
    const Services::OtaViewState& otaState,
    const Services::StationStore::ViewState& stationState,
    std::uint32_t now) {

    if (latestLabel == nullptr || statsLabel == nullptr || scheduleLabel == nullptr ||
        memoryLabel == nullptr || systemLabel == nullptr || stateLabel == nullptr) {
        return;
    }

    if (radioState.noiseHistoryCount == 0U) {
        lv_label_set_text_fmt(
            latestLabel,
            App::Localization::text(
                "RSSI pozadi %.3f MHz: cekam na mereni",
                "Background RSSI %.3f MHz: waiting for measurement"),
            static_cast<double>(radioState.loraFrequencyMHz));
        lv_label_set_text(
            statsLabel,
            App::Localization::text(
                "Modra = prumer, oranzova = spicka",
                "Blue = average, orange = peak"));
    } else {
        lv_label_set_text_fmt(
            latestLabel,
            App::Localization::text(
                "%.3f MHz: %.1f dBm | spicka %.1f",
                "%.3f MHz: %.1f dBm | peak %.1f"),
            static_cast<double>(radioState.loraFrequencyMHz),
            static_cast<double>(radioState.noiseLatestAverageDbm),
            static_cast<double>(radioState.noiseLatestPeakDbm));
        lv_label_set_text_fmt(
            statsLabel,
            App::Localization::text(
                "Historie %u/%u | prumer %.1f | min %.1f | max %.1f",
                "History %u/%u | average %.1f | min %.1f | max %.1f"),
            static_cast<unsigned>(radioState.noiseHistoryCount),
            static_cast<unsigned>(AppConfig::RADIO_NOISE_HISTORY_LENGTH),
            static_cast<double>(radioState.noiseHistoryAverageDbm),
            static_cast<double>(radioState.noiseHistoryMinDbm),
            static_cast<double>(radioState.noiseHistoryMaxDbm));
    }

    if (!radioState.initialized) {
        lv_label_set_text(
            scheduleLabel,
            App::Localization::text(
                "Radio neni dostupne - mereni pozastaveno",
                "Radio unavailable - measurement paused"));
        lv_obj_set_style_text_color(scheduleLabel, lv_color_hex(0xFF6B6B), 0);
    } else if (radioState.noiseMeasurementActive) {
        lv_label_set_text_fmt(
            scheduleLabel,
            App::Localization::text(
                "Merim %u/%u vzorku; RX zustava aktivni",
                "Measuring %u/%u samples; RX remains active"),
            static_cast<unsigned>(radioState.noiseBurstProgress),
            static_cast<unsigned>(AppConfig::RADIO_NOISE_BURST_SAMPLES));
        lv_obj_set_style_text_color(scheduleLabel, lv_color_hex(0x42D392), 0);
    } else {
        const std::uint32_t remaining = secondsUntil(now, radioState.noiseNextMeasurementAtMs);
        lv_label_set_text_fmt(
            scheduleLabel,
            App::Localization::text(
                "Dalsi za %lu:%02lu | interval 5 min | 20 bodu",
                "Next in %lu:%02lu | 5 min interval | 20 points"),
            static_cast<unsigned long>(remaining / 60U),
            static_cast<unsigned long>(remaining % 60U));
        lv_obj_set_style_text_color(scheduleLabel, lv_color_hex(0x92A7C7), 0);
    }

    const bool lowHeap = systemState.freeInternalBytes <
        AppConfig::SYSTEM_DIAGNOSTICS_LOW_HEAP_BYTES;
    lv_label_set_text_fmt(
        memoryLabel,
        App::Localization::text(
            "Heap %u KB | blok %u KB | minimum %u KB",
            "Heap %u KB | block %u KB | minimum %u KB"),
        kib(systemState.freeInternalBytes),
        kib(systemState.largestInternalBlockBytes),
        kib(systemState.minimumFreeInternalBytes));
    lv_obj_set_style_text_color(
        memoryLabel,
        lv_color_hex(lowHeap ? 0xFF6B6B : 0xBDCAE0),
        0);

    const bool lowStack = systemState.loopStackMinimumFreeBytes <
        AppConfig::SYSTEM_DIAGNOSTICS_LOW_STACK_BYTES;
    if (systemState.psramAvailable) {
        lv_label_set_text_fmt(
            systemLabel,
            App::Localization::text(
                "PSRAM %u KB | stack min %u B | stanice %u/%u",
                "PSRAM %u KB | stack min %u B | stations %u/%u"),
            kib(systemState.freePsramBytes),
            static_cast<unsigned>(systemState.loopStackMinimumFreeBytes),
            static_cast<unsigned>(stationState.count),
            static_cast<unsigned>(Services::StationStore::MAX_STATIONS));
    } else {
        lv_label_set_text_fmt(
            systemLabel,
            App::Localization::text(
                "PSRAM NENI | stack min %u B | stanice %u/%u",
                "PSRAM MISSING | stack min %u B | stations %u/%u"),
            static_cast<unsigned>(systemState.loopStackMinimumFreeBytes),
            static_cast<unsigned>(stationState.count),
            static_cast<unsigned>(Services::StationStore::MAX_STATIONS));
    }
    lv_obj_set_style_text_color(
        systemLabel,
        lv_color_hex((lowStack || !systemState.psramAvailable) ? 0xFFB454 : 0xBDCAE0),
        0);

    char uptime[24];
    formatUptime(systemState.uptimeSeconds, uptime, sizeof(uptime));
    const char* otaText = otaState.accessPointActive
        ? (otaState.uploadActive ? "UPLOAD" : "AP")
        : (otaState.manuallyStopped ? "STOP" : (otaState.enabled ? "WAIT" : "OFF"));
    lv_label_set_text_fmt(
        stateLabel,
        App::Localization::text(
            "Beh %s | reset %s | OTA %s | TX fronta %u/%u",
            "Up %s | reset %s | OTA %s | TX queue %u/%u"),
        uptime,
        systemState.resetReason,
        otaText,
        static_cast<unsigned>(radioState.txQueueDepth),
        static_cast<unsigned>(radioState.txQueueMaximumDepth));

    if (renderedRadioRevision != radioState.noiseHistoryRevision) {
        renderedRadioRevision = radioState.noiseHistoryRevision;
        refreshChart(radioState);
    }
}

}  // namespace DiagnosticsScreen
}  // namespace Ui
