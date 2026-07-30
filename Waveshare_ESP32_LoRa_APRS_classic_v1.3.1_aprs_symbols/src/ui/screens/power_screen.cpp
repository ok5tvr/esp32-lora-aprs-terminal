#include "ui/screens/power_screen.h"

#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "ui/ui_components.h"

namespace Ui {
namespace PowerScreen {
namespace {

lv_obj_t* stateLabel = nullptr;
lv_obj_t* batteryLabel = nullptr;
lv_obj_t* chargerLabel = nullptr;
lv_obj_t* chargeSettingsLabel = nullptr;
lv_obj_t* usbLabel = nullptr;
lv_obj_t* systemLabel = nullptr;
lv_obj_t* eventLabel = nullptr;
std::uint32_t renderedRevision = 0xFFFFFFFFU;

void formatVoltageCz(char* output, std::size_t capacity, std::uint16_t millivolts) {
    if (output == nullptr || capacity == 0U) {
        return;
    }
    std::snprintf(output, capacity, "%.2f", static_cast<double>(millivolts) / 1000.0);
    if (char* decimal = std::strchr(output, '.')) {
        *decimal = ',';
    }
}

void styleLine(lv_obj_t* label, lv_coord_t y, lv_color_t color = lv_color_hex(0xBDCAE0)) {
    lv_obj_set_width(label, 452);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 14, y);
}

}  // namespace

void create() {
    resetScreen();
    createHeader("Napajeni");

    stateLabel = lv_label_create(lv_scr_act());
    styleLine(stateLabel, 54);
    lv_obj_set_style_text_font(stateLabel, &lv_font_montserrat_18, 0);

    batteryLabel = lv_label_create(lv_scr_act());
    styleLine(batteryLabel, 82);

    chargerLabel = lv_label_create(lv_scr_act());
    styleLine(chargerLabel, 109);

    chargeSettingsLabel = lv_label_create(lv_scr_act());
    styleLine(chargeSettingsLabel, 136);

    usbLabel = lv_label_create(lv_scr_act());
    styleLine(usbLabel, 163);

    systemLabel = lv_label_create(lv_scr_act());
    styleLine(systemLabel, 190);

    eventLabel = lv_label_create(lv_scr_act());
    styleLine(eventLabel, 217, lv_color_hex(0x92A7C7));

    renderedRevision = 0xFFFFFFFFU;
}

void update(const Services::PowerService::ViewState& state) {
    if (stateLabel == nullptr || renderedRevision == state.revision) {
        return;
    }
    renderedRevision = state.revision;

    if (!state.available) {
        lv_label_set_text(stateLabel, "AXP2101 neni dostupny");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xF05B67), 0);
        lv_label_set_text(batteryLabel, "Akumulator: -- | nabiti: -- | napeti: --");
        lv_label_set_text(chargerLabel, "Stav nabijeni: --");
        lv_label_set_text(chargeSettingsLabel, "Nastaveny proud: -- | cilove napeti: --");
        lv_label_set_text(usbLabel, "USB-C: -- | VBUS: --");
        lv_label_set_text(systemLabel, "System: -- | teplota PMIC: --");
        lv_label_set_text(eventLabel, state.lastEvent);
        return;
    }

    lv_label_set_text_fmt(stateLabel, "Stav: %s", state.operatingText);
    if (state.criticalBattery) {
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xF05B67), 0);
    } else if (state.charging) {
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x42D392), 0);
    } else if (state.vbusConnected) {
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x56C7FF), 0);
    } else {
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xF4F7FF), 0);
    }

    char text[192];
    char batteryVoltage[12];
    char vbusVoltage[12];
    char systemVoltage[12];
    char targetVoltage[12];
    formatVoltageCz(batteryVoltage, sizeof(batteryVoltage), state.batteryVoltageMv);
    formatVoltageCz(vbusVoltage, sizeof(vbusVoltage), state.vbusVoltageMv);
    formatVoltageCz(systemVoltage, sizeof(systemVoltage), state.systemVoltageMv);
    formatVoltageCz(targetVoltage, sizeof(targetVoltage), state.targetChargeVoltageMv);

    if (state.batteryConnected) {
        if (state.batteryPercentValid) {
            std::snprintf(
                text,
                sizeof(text),
                "Akumulator: pripojen | nabiti: %u %% | napeti: %s V",
                static_cast<unsigned>(state.batteryPercent),
                batteryVoltage);
        } else {
            std::snprintf(
                text,
                sizeof(text),
                "Akumulator: pripojen | nabiti: -- | napeti: %s V",
                batteryVoltage);
        }
    } else {
        std::snprintf(text, sizeof(text), "Akumulator: nepripojen | nabiti: -- | napeti: --");
    }
    lv_label_set_text(batteryLabel, text);

    std::snprintf(text, sizeof(text), "Stav nabijeni: %s", state.chargerText);
    lv_label_set_text(chargerLabel, text);

    if (state.configuredChargeCurrentMa > 0U && state.targetChargeVoltageMv > 0U) {
        std::snprintf(
            text,
            sizeof(text),
            "Nastaveny proud: %u mA | cilove napeti: %s V",
            static_cast<unsigned>(state.configuredChargeCurrentMa),
            targetVoltage);
    } else {
        std::snprintf(text, sizeof(text), "Nastaveny proud: -- | cilove napeti: --");
    }
    lv_label_set_text(chargeSettingsLabel, text);

    std::snprintf(
        text,
        sizeof(text),
        "USB-C: %s | VBUS: %s V%s",
        state.vbusConnected ? "pripojeno" : "nepripojeno",
        vbusVoltage,
        state.vbusConnected && !state.vbusGood ? " (nestabilni)" : "");
    lv_label_set_text(usbLabel, text);

    if (state.pmicTemperatureValid) {
        std::snprintf(
            text,
            sizeof(text),
            "System: %s V | teplota PMIC: %.1f C%s",
            systemVoltage,
            static_cast<double>(state.pmicTemperatureC),
            state.criticalBattery ? " | KRITICKA BATERIE" : "");
    } else {
        std::snprintf(
            text,
            sizeof(text),
            "System: %s V | teplota PMIC: --%s",
            systemVoltage,
            state.criticalBattery ? " | KRITICKA BATERIE" : "");
    }
    lv_label_set_text(systemLabel, text);

    std::snprintf(text, sizeof(text), "Posledni udalost: %s", state.lastEvent);
    lv_label_set_text(eventLabel, text);
}

}  // namespace PowerScreen
}  // namespace Ui
