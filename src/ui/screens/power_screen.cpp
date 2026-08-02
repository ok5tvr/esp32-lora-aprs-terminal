#include "ui/screens/power_screen.h"

#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "app_config.h"
#include "app/localization.h"
#include "ui/ui_components.h"

namespace Ui {
namespace PowerScreen {
namespace {

lv_obj_t* stateLabel = nullptr;
lv_obj_t* batteryLabel = nullptr;
lv_obj_t* chargerLabel = nullptr;
lv_obj_t* inputLabel = nullptr;
lv_obj_t* historyLabel = nullptr;
lv_obj_t* historyChart = nullptr;
lv_chart_series_t* chargingSeries = nullptr;
lv_chart_series_t* dischargingSeries = nullptr;
lv_chart_series_t* usbSeries = nullptr;
std::uint32_t renderedRevision = 0xFFFFFFFFU;
std::uint32_t renderedHistoryRevision = 0xFFFFFFFFU;

enum class ChartMode : std::uint8_t {
    Charging = 0,
    Discharging,
    Usb
};

void formatVoltage(char* output, std::size_t capacity, std::uint16_t millivolts) {
    if (output == nullptr || capacity == 0U) {
        return;
    }
    std::snprintf(output, capacity, "%.2f", static_cast<double>(millivolts) / 1000.0);
    if (!App::Localization::isEnglish()) {
        if (char* decimal = std::strchr(output, '.')) {
            *decimal = ',';
        }
    }
}

void styleLine(lv_obj_t* label, lv_coord_t y, lv_color_t color = lv_color_hex(0xBDCAE0)) {
    lv_obj_set_width(label, 452);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 14, y);
}

ChartMode chartMode(Services::PowerService::HistoryMode mode) {
    switch (mode) {
        case Services::PowerService::HistoryMode::Charging:
            return ChartMode::Charging;
        case Services::PowerService::HistoryMode::Discharging:
            return ChartMode::Discharging;
        case Services::PowerService::HistoryMode::UsbPower:
        case Services::PowerService::HistoryMode::Standby:
        case Services::PowerService::HistoryMode::Unknown:
        default:
            return ChartMode::Usb;
    }
}

void refreshHistoryChart(const Services::PowerService::ViewState& state) {
    if (historyChart == nullptr || chargingSeries == nullptr ||
        dischargingSeries == nullptr || usbSeries == nullptr) {
        return;
    }

    lv_coord_t values[AppConfig::POWER_HISTORY_LENGTH] = {};
    ChartMode modes[AppConfig::POWER_HISTORY_LENGTH] = {};
    bool valid[AppConfig::POWER_HISTORY_LENGTH] = {};

    const std::size_t count = state.powerHistoryCount;
    if (count == 1U ||
        (count > 1U && state.powerHistoryAtMinute[count - 1U] == state.powerHistoryAtMinute[0])) {
        // Two identical points make a single restored value visible even with
        // chart indicators disabled.
        const std::size_t firstVisible = AppConfig::POWER_HISTORY_LENGTH - 2U;
        for (std::size_t index = firstVisible; index < AppConfig::POWER_HISTORY_LENGTH; ++index) {
            values[index] = static_cast<lv_coord_t>(state.powerHistoryPercent[count - 1U]);
            modes[index] = chartMode(state.powerHistoryMode[count - 1U]);
            valid[index] = true;
        }
    } else if (count > 1U) {
        const std::uint32_t firstMinute = state.powerHistoryAtMinute[0];
        const std::uint32_t lastMinute = state.powerHistoryAtMinute[count - 1U];
        const std::uint32_t spanMinutes = lastMinute - firstMinute;
        std::size_t left = 0U;

        for (std::size_t chartIndex = 0; chartIndex < AppConfig::POWER_HISTORY_LENGTH; ++chartIndex) {
            const std::uint32_t targetMinute = firstMinute + static_cast<std::uint32_t>(
                (static_cast<std::uint64_t>(spanMinutes) * chartIndex) /
                (AppConfig::POWER_HISTORY_LENGTH - 1U));
            while (left + 1U < count &&
                   state.powerHistoryAtMinute[left + 1U] <= targetMinute) {
                ++left;
            }

            const std::size_t right = left + 1U < count ? left + 1U : left;
            const std::uint32_t leftMinute = state.powerHistoryAtMinute[left];
            const std::uint32_t rightMinute = state.powerHistoryAtMinute[right];
            const int leftPercent = state.powerHistoryPercent[left];
            const int rightPercent = state.powerHistoryPercent[right];

            int interpolated = leftPercent;
            ChartMode mode = chartMode(state.powerHistoryMode[left]);
            if (right > left && targetMinute > leftMinute && rightMinute > leftMinute) {
                const std::uint32_t elapsed = targetMinute - leftMinute;
                const std::uint32_t duration = rightMinute - leftMinute;
                interpolated = leftPercent + static_cast<int>(
                    (static_cast<std::int64_t>(rightPercent - leftPercent) * elapsed) /
                    duration);
            }

            values[chartIndex] = static_cast<lv_coord_t>(interpolated);
            modes[chartIndex] = mode;
            valid[chartIndex] = true;
        }
    }

    for (std::size_t chartIndex = 0; chartIndex < AppConfig::POWER_HISTORY_LENGTH; ++chartIndex) {
        const lv_coord_t value = valid[chartIndex]
            ? values[chartIndex]
            : LV_CHART_POINT_NONE;
        const bool adjacentCharging = valid[chartIndex] &&
            ((chartIndex > 0U && valid[chartIndex - 1U] && modes[chartIndex - 1U] == ChartMode::Charging) ||
             (chartIndex + 1U < AppConfig::POWER_HISTORY_LENGTH && valid[chartIndex + 1U] &&
              modes[chartIndex + 1U] == ChartMode::Charging));
        const bool adjacentDischarging = valid[chartIndex] &&
            ((chartIndex > 0U && valid[chartIndex - 1U] && modes[chartIndex - 1U] == ChartMode::Discharging) ||
             (chartIndex + 1U < AppConfig::POWER_HISTORY_LENGTH && valid[chartIndex + 1U] &&
              modes[chartIndex + 1U] == ChartMode::Discharging));
        const bool adjacentUsb = valid[chartIndex] &&
            ((chartIndex > 0U && valid[chartIndex - 1U] && modes[chartIndex - 1U] == ChartMode::Usb) ||
             (chartIndex + 1U < AppConfig::POWER_HISTORY_LENGTH && valid[chartIndex + 1U] &&
              modes[chartIndex + 1U] == ChartMode::Usb));
        lv_chart_set_value_by_id(
            historyChart,
            chargingSeries,
            static_cast<std::uint16_t>(chartIndex),
            value != LV_CHART_POINT_NONE &&
                    (modes[chartIndex] == ChartMode::Charging || adjacentCharging)
                ? value
                : LV_CHART_POINT_NONE);
        lv_chart_set_value_by_id(
            historyChart,
            dischargingSeries,
            static_cast<std::uint16_t>(chartIndex),
            value != LV_CHART_POINT_NONE &&
                    (modes[chartIndex] == ChartMode::Discharging || adjacentDischarging)
                ? value
                : LV_CHART_POINT_NONE);
        lv_chart_set_value_by_id(
            historyChart,
            usbSeries,
            static_cast<std::uint16_t>(chartIndex),
            value != LV_CHART_POINT_NONE &&
                    (modes[chartIndex] == ChartMode::Usb || adjacentUsb)
                ? value
                : LV_CHART_POINT_NONE);
    }
    lv_chart_refresh(historyChart);
}

}  // namespace

void create() {
    resetScreen();
    createHeader(App::Localization::text("Napajeni", "Power"));

    stateLabel = lv_label_create(lv_scr_act());
    styleLine(stateLabel, 53, lv_color_hex(0xF4F7FF));
    lv_obj_set_style_text_font(stateLabel, &lv_font_montserrat_16, 0);

    batteryLabel = lv_label_create(lv_scr_act());
    styleLine(batteryLabel, 78);

    chargerLabel = lv_label_create(lv_scr_act());
    styleLine(chargerLabel, 101);

    inputLabel = lv_label_create(lv_scr_act());
    styleLine(inputLabel, 124);

    historyLabel = lv_label_create(lv_scr_act());
    styleLine(historyLabel, 147, lv_color_hex(0x92A7C7));

    historyChart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(historyChart, 452, 78);
    lv_obj_align(historyChart, LV_ALIGN_TOP_MID, 0, 169);
    lv_chart_set_type(historyChart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(historyChart, AppConfig::POWER_HISTORY_LENGTH);
    lv_chart_set_range(historyChart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(historyChart, 5, 6);
    lv_obj_set_style_bg_color(historyChart, lv_color_hex(0x101D31), 0);
    lv_obj_set_style_bg_opa(historyChart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(historyChart, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(historyChart, 1, 0);
    lv_obj_set_style_line_color(historyChart, lv_color_hex(0x263A59), LV_PART_MAIN);
    lv_obj_set_style_line_width(historyChart, 1, LV_PART_MAIN);
    lv_obj_set_style_line_width(historyChart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(historyChart, 0, LV_PART_INDICATOR);

    chargingSeries = lv_chart_add_series(
        historyChart,
        lv_color_hex(0x42D392),
        LV_CHART_AXIS_PRIMARY_Y);
    dischargingSeries = lv_chart_add_series(
        historyChart,
        lv_color_hex(0xFFB454),
        LV_CHART_AXIS_PRIMARY_Y);
    usbSeries = lv_chart_add_series(
        historyChart,
        lv_color_hex(0x56C7FF),
        LV_CHART_AXIS_PRIMARY_Y);

    renderedRevision = 0xFFFFFFFFU;
    renderedHistoryRevision = 0xFFFFFFFFU;
}

void update(const Services::PowerService::ViewState& state) {
    if (stateLabel == nullptr) {
        return;
    }

    if (renderedRevision != state.revision) {
        renderedRevision = state.revision;

        if (!state.available) {
            lv_label_set_text(stateLabel, App::Localization::text("AXP2101 neni dostupny", "AXP2101 is unavailable"));
            lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xF05B67), 0);
            lv_label_set_text(batteryLabel, App::Localization::text("Akumulator: -- | nabiti: -- | napeti: --", "Battery: -- | charge: -- | voltage: --"));
            lv_label_set_text(chargerLabel, App::Localization::text("Nabijeni: -- | proud/cil: --", "Charging: -- | current/target: --"));
            lv_label_set_text(inputLabel, App::Localization::text("USB-C: -- | system: -- | PMIC: --", "USB-C: -- | system: -- | PMIC: --"));
            lv_label_set_text(historyLabel, state.lastEvent);
        } else {
            lv_label_set_text_fmt(
                stateLabel,
                App::Localization::text("Stav: %s | %s", "Status: %s | %s"),
                state.operatingText,
                state.lastEvent);
            if (state.criticalBattery) {
                lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xF05B67), 0);
            } else if (state.charging) {
                lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x42D392), 0);
            } else if (state.vbusConnected) {
                lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x56C7FF), 0);
            } else {
                lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xF4F7FF), 0);
            }

            char text[224];
            char batteryVoltage[12];
            char vbusVoltage[12];
            char systemVoltage[12];
            char targetVoltage[12];
            formatVoltage(batteryVoltage, sizeof(batteryVoltage), state.batteryVoltageMv);
            formatVoltage(vbusVoltage, sizeof(vbusVoltage), state.vbusVoltageMv);
            formatVoltage(systemVoltage, sizeof(systemVoltage), state.systemVoltageMv);
            formatVoltage(targetVoltage, sizeof(targetVoltage), state.targetChargeVoltageMv);

            if (state.batteryConnected) {
                if (state.batteryPercentValid) {
                    std::snprintf(
                        text,
                        sizeof(text),
                        App::Localization::text("Akumulator: %u %% | %s V", "Battery: %u %% | %s V"),
                        static_cast<unsigned>(state.batteryPercent),
                        batteryVoltage);
                } else {
                    std::snprintf(
                        text,
                        sizeof(text),
                        App::Localization::text("Akumulator: -- | %s V", "Battery: -- | %s V"),
                        batteryVoltage);
                }
            } else {
                std::snprintf(text, sizeof(text), App::Localization::text("Akumulator: nepripojen", "Battery: disconnected"));
            }
            lv_label_set_text(batteryLabel, text);

            if (state.configuredChargeCurrentMa > 0U && state.targetChargeVoltageMv > 0U) {
                std::snprintf(
                    text,
                    sizeof(text),
                    App::Localization::text("Nabijeni: %s | %u mA / %s V", "Charging: %s | %u mA / %s V"),
                    state.chargerText,
                    static_cast<unsigned>(state.configuredChargeCurrentMa),
                    targetVoltage);
            } else {
                std::snprintf(
                    text,
                    sizeof(text),
                    App::Localization::text("Nabijeni: %s | proud/cil: --", "Charging: %s | current/target: --"),
                    state.chargerText);
            }
            lv_label_set_text(chargerLabel, text);

            char temperature[16] = "--";
            if (state.pmicTemperatureValid) {
                std::snprintf(temperature, sizeof(temperature), "%.1f C", static_cast<double>(state.pmicTemperatureC));
                if (!App::Localization::isEnglish()) {
                    if (char* decimal = std::strchr(temperature, '.')) {
                        *decimal = ',';
                    }
                }
            }
            std::snprintf(
                text,
                sizeof(text),
                App::Localization::text("USB-C: %s %s V | SYS %s V | PMIC %s", "USB-C: %s %s V | SYS %s V | PMIC %s"),
                state.vbusConnected
                    ? App::Localization::text("ano", "yes")
                    : App::Localization::text("ne", "no"),
                vbusVoltage,
                systemVoltage,
                temperature);
            lv_label_set_text(inputLabel, text);

            if (state.powerHistoryCount == 0U) {
                lv_label_set_text(
                    historyLabel,
                    App::Localization::text("Historie baterie: cekam | krok 1 % | ulozeni NVS", "Battery history: waiting | 1% step | NVS storage"));
            } else {
                const std::size_t last = state.powerHistoryCount - 1U;
                const std::uint32_t spanMinutes =
                    state.powerHistoryAtMinute[last] - state.powerHistoryAtMinute[0];
                lv_label_set_text_fmt(
                    historyLabel,
                    App::Localization::text("Historie %lu:%02lu h | %u/%u | krok 1 %% | NVS", "History %lu:%02lu h | %u/%u | 1%% step | NVS"),
                    static_cast<unsigned long>(spanMinutes / 60U),
                    static_cast<unsigned long>(spanMinutes % 60U),
                    static_cast<unsigned>(state.powerHistoryCount),
                    static_cast<unsigned>(AppConfig::POWER_HISTORY_LENGTH));
            }
        }
    }

    if (renderedHistoryRevision != state.powerHistoryRevision) {
        renderedHistoryRevision = state.powerHistoryRevision;
        refreshHistoryChart(state);
    }
}

}  // namespace PowerScreen
}  // namespace Ui
