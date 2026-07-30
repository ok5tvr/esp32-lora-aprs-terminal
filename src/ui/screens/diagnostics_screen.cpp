#include "ui/screens/diagnostics_screen.h"

#include <cmath>
#include <cstdio>
#include <lvgl.h>

#include "app_config.h"
#include "lora_profile.h"
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
std::uint32_t renderedRevision = 0xFFFFFFFFUL;

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

}  // namespace

void create() {
    resetScreen();
    createHeader("Diagnostika");

    lv_obj_t* summary = lv_obj_create(lv_scr_act());
    lv_obj_set_size(summary, 452, 64);
    lv_obj_align(summary, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_color(summary, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(summary, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(summary, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(summary, 1, 0);
    lv_obj_set_style_radius(summary, 10, 0);
    lv_obj_set_style_pad_all(summary, 7, 0);
    lv_obj_clear_flag(summary, LV_OBJ_FLAG_SCROLLABLE);

    latestLabel = lv_label_create(summary);
    lv_obj_set_style_text_font(latestLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(latestLabel, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(latestLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    statsLabel = lv_label_create(summary);
    lv_obj_set_style_text_font(statsLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(statsLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(statsLabel, LV_ALIGN_TOP_LEFT, 0, 23);

    scheduleLabel = lv_label_create(summary);
    lv_obj_set_style_text_font(scheduleLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(scheduleLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(scheduleLabel, LV_ALIGN_TOP_LEFT, 0, 43);

    chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart, 452, 126);
    lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 122);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, AppConfig::RADIO_NOISE_HISTORY_LENGTH);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -165, -40);
    lv_chart_set_div_line_count(chart, 5, 4);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x101D31), 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(chart, 1, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x263A59), LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
    lv_obj_set_style_size(chart, 4, LV_PART_INDICATOR);

    averageSeries = lv_chart_add_series(
        chart,
        lv_color_hex(0x56C7FF),
        LV_CHART_AXIS_PRIMARY_Y);
    peakSeries = lv_chart_add_series(
        chart,
        lv_color_hex(0xFFB454),
        LV_CHART_AXIS_PRIMARY_Y);

    renderedRevision = 0xFFFFFFFFUL;
}

void update(const Services::RadioService::ViewState& state, std::uint32_t now) {
    if (latestLabel == nullptr || statsLabel == nullptr || scheduleLabel == nullptr) {
        return;
    }

    if (state.noiseHistoryCount == 0U) {
        lv_label_set_text_fmt(
            latestLabel,
            "RSSI pozadi %.3f MHz: cekam na prvni mereni",
            static_cast<double>(LoRaProfile::FREQUENCY_MHZ));
        lv_label_set_text(
            statsLabel,
            "Modra = prumer, oranzova = spicka v mericim okne");
    } else {
        lv_label_set_text_fmt(
            latestLabel,
            "Posledni: %.1f dBm | spicka %.1f dBm",
            static_cast<double>(state.noiseLatestAverageDbm),
            static_cast<double>(state.noiseLatestPeakDbm));
        lv_label_set_text_fmt(
            statsLabel,
            "Historie %u/%u | prumer %.1f | min %.1f | max %.1f dBm",
            static_cast<unsigned>(state.noiseHistoryCount),
            static_cast<unsigned>(AppConfig::RADIO_NOISE_HISTORY_LENGTH),
            static_cast<double>(state.noiseHistoryAverageDbm),
            static_cast<double>(state.noiseHistoryMinDbm),
            static_cast<double>(state.noiseHistoryMaxDbm));
    }

    if (!state.initialized) {
        lv_label_set_text(scheduleLabel, "Radio neni dostupne - mereni je pozastaveno");
        lv_obj_set_style_text_color(scheduleLabel, lv_color_hex(0xFF6B6B), 0);
    } else if (state.noiseMeasurementActive) {
        lv_label_set_text_fmt(
            scheduleLabel,
            "Merim %u/%u vzorku; RX zustava aktivni",
            static_cast<unsigned>(state.noiseBurstProgress),
            static_cast<unsigned>(AppConfig::RADIO_NOISE_BURST_SAMPLES));
        lv_obj_set_style_text_color(scheduleLabel, lv_color_hex(0x42D392), 0);
    } else {
        const std::uint32_t remaining = secondsUntil(now, state.noiseNextMeasurementAtMs);
        lv_label_set_text_fmt(
            scheduleLabel,
            "Dalsi mereni za %lu:%02lu | interval 5 min | 20 bodu",
            static_cast<unsigned long>(remaining / 60U),
            static_cast<unsigned long>(remaining % 60U));
        lv_obj_set_style_text_color(scheduleLabel, lv_color_hex(0x92A7C7), 0);
    }

    if (renderedRevision != state.noiseHistoryRevision) {
        renderedRevision = state.noiseHistoryRevision;
        refreshChart(state);
    }
}

}  // namespace DiagnosticsScreen
}  // namespace Ui
