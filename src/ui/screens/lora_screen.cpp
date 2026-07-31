#include "ui/screens/lora_screen.h"

#include <cstdio>
#include <lvgl.h>

#include "lora_profile.h"
#include "app/localization.h"
#include "services/tx_queue.h"
#include "ui/ui_components.h"

namespace Ui {
namespace LoRaScreen {
namespace {

lv_obj_t* stateLabel = nullptr;
lv_obj_t* paramsLabel = nullptr;
lv_obj_t* packetLabel = nullptr;
lv_obj_t* signalLabel = nullptr;
lv_obj_t* countersLabel = nullptr;
lv_obj_t* queueLabel = nullptr;
lv_obj_t* recoveryLabel = nullptr;
lv_obj_t* messageLabel = nullptr;

}  // namespace

void create() {
    resetScreen();
    createHeader("LoRa APRS");

    lv_obj_t* card = lv_obj_create(lv_scr_act());
    lv_obj_set_size(card, 450, 194);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 9, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    stateLabel = lv_label_create(card);
    lv_obj_set_style_text_font(stateLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(stateLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    paramsLabel = lv_label_create(card);
    lv_obj_set_width(paramsLabel, 426);
    lv_label_set_long_mode(paramsLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(paramsLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(paramsLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(paramsLabel, LV_ALIGN_TOP_LEFT, 0, 24);

    packetLabel = lv_label_create(card);
    lv_obj_set_width(packetLabel, 426);
    lv_obj_set_height(packetLabel, 39);
    lv_label_set_long_mode(packetLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(packetLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(packetLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(packetLabel, LV_ALIGN_TOP_LEFT, 0, 46);

    signalLabel = lv_label_create(card);
    lv_obj_set_style_text_font(signalLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(signalLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(signalLabel, LV_ALIGN_TOP_LEFT, 0, 88);

    countersLabel = lv_label_create(card);
    lv_obj_set_style_text_font(countersLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(countersLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(countersLabel, LV_ALIGN_TOP_LEFT, 0, 110);

    queueLabel = lv_label_create(card);
    lv_obj_set_style_text_font(queueLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(queueLabel, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(queueLabel, LV_ALIGN_TOP_LEFT, 0, 132);

    recoveryLabel = lv_label_create(card);
    lv_obj_set_width(recoveryLabel, 426);
    lv_label_set_long_mode(recoveryLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(recoveryLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(recoveryLabel, lv_color_hex(0xFFB454), 0);
    lv_obj_align(recoveryLabel, LV_ALIGN_TOP_LEFT, 0, 154);

    messageLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(
        messageLabel,
        App::Localization::text(
            "OK = test do centralni TX fronty",
            "OK = queue a test packet in the central TX queue"));
    lv_obj_set_style_text_font(messageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(messageLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(messageLabel, LV_ALIGN_BOTTOM_MID, 0, -72);
}

void update(const Services::RadioService::ViewState& state) {
    if (stateLabel == nullptr || paramsLabel == nullptr || packetLabel == nullptr ||
        signalLabel == nullptr || countersLabel == nullptr || queueLabel == nullptr ||
        recoveryLabel == nullptr) {
        return;
    }

    lv_label_set_text_fmt(
        paramsLabel,
        "%s | %.3f MHz | BW %.1f | SF%u | CR4/%u | %d dBm%s",
        state.loraPreset == App::LoRaPreset::CzeAprs
            ? "CZE"
            : App::Localization::text("VLASTNI", "CUSTOM"),
        static_cast<double>(state.loraFrequencyMHz),
        static_cast<double>(state.loraBandwidthKHz),
        static_cast<unsigned>(state.loraSpreadingFactor),
        static_cast<unsigned>(state.loraCodingRate),
        static_cast<int>(state.loraOutputPowerDbm),
        state.loraConfigurationPending
            ? App::Localization::text(" | CEKA", " | PENDING")
            : "");

    if (!state.initialized) {
        lv_label_set_text_fmt(stateLabel, App::Localization::text("Stav: CHYBA (%d)", "Status: ERROR (%d)"), static_cast<int>(state.lastError));
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xFF6B6B), 0);
    } else if (state.transmitting) {
        lv_label_set_text_fmt(stateLabel, App::Localization::text("Stav: TX %s", "Status: TX %s"), state.lastTxSource);
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xFFB454), 0);
    } else if (state.receiving) {
        lv_label_set_text(stateLabel, App::Localization::text("Stav: RX - prijem aktivni", "Status: RX - reception active"));
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x42D392), 0);
    } else {
        lv_label_set_text(stateLabel, App::Localization::text("Stav: pripravuji radio", "Status: preparing radio"));
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x92A7C7), 0);
    }

    lv_label_set_text(packetLabel,
        state.lastPacketText[0] != '\0'
            ? state.lastPacketText
            : App::Localization::text("Cekam na prvni paket...", "Waiting for the first packet..."));
    lv_label_set_text_fmt(
        signalLabel,
        "RSSI %.1f dBm | SNR %.1f dB | FErr %.0f Hz%s",
        static_cast<double>(state.lastRssiDbm),
        static_cast<double>(state.lastSnrDb),
        static_cast<double>(state.lastFrequencyErrorHz),
        state.lastPacketHadOeHeader ? " | OE" : "");
    lv_label_set_text_fmt(
        countersLabel,
        "RX %lu | APRS %lu | DECERR %lu | RXERR %lu | TX %lu",
        static_cast<unsigned long>(state.receivedPackets),
        static_cast<unsigned long>(state.validAprsPackets),
        static_cast<unsigned long>(state.decodeErrors),
        static_cast<unsigned long>(state.receiveErrors),
        static_cast<unsigned long>(state.transmittedPackets));
    lv_label_set_text_fmt(
        queueLabel,
        App::Localization::text(
            "TX fronta %u/%u (max %u) | vlozeno %lu | nahrazeno %lu | drop %lu",
            "TX queue %u/%u (max %u) | queued %lu | replaced %lu | dropped %lu"),
        static_cast<unsigned>(state.txQueueDepth),
        static_cast<unsigned>(Services::TxQueue::CAPACITY),
        static_cast<unsigned>(state.txQueueMaximumDepth),
        static_cast<unsigned long>(state.txQueueEnqueued),
        static_cast<unsigned long>(state.txQueueReplaced),
        static_cast<unsigned long>(state.txQueueDrops));
    lv_label_set_text_fmt(
        recoveryLabel,
        App::Localization::text(
            "Autoobnova %lu OK / %lu selhani | TX timeout %lu | %s",
            "Auto recovery %lu OK / %lu failed | TX timeout %lu | %s"),
        static_cast<unsigned long>(state.successfulRecoveries),
        static_cast<unsigned long>(state.recoveryFailures),
        static_cast<unsigned long>(state.transmitTimeouts),
        state.lastRecoveryText);
}

void setMessage(const char* text) {
    if (messageLabel != nullptr) {
        lv_label_set_text(messageLabel, text);
    }
}

}  // namespace LoRaScreen
}  // namespace Ui
