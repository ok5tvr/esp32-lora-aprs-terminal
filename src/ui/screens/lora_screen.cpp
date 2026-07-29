#include "ui/screens/lora_screen.h"

#include <cstdio>
#include <lvgl.h>

#include "lora_profile.h"
#include "ui/ui_components.h"

namespace Ui {
namespace LoRaScreen {
namespace {

lv_obj_t* stateLabel = nullptr;
lv_obj_t* packetLabel = nullptr;
lv_obj_t* signalLabel = nullptr;
lv_obj_t* countersLabel = nullptr;
lv_obj_t* messageLabel = nullptr;

}  // namespace

void create() {
    resetScreen();
    createHeader("LoRa APRS - RA-02");

    lv_obj_t* card = lv_obj_create(lv_scr_act());
    lv_obj_set_size(card, 450, 188);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    stateLabel = lv_label_create(card);
    lv_obj_set_style_text_font(stateLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(stateLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* params = lv_label_create(card);
    char paramsText[96];
    std::snprintf(
        paramsText,
        sizeof(paramsText),
        "%.3f MHz | BW %.0f kHz | SF%u | CR 4/%u",
        static_cast<double>(LoRaProfile::FREQUENCY_MHZ),
        static_cast<double>(LoRaProfile::BANDWIDTH_KHZ),
        static_cast<unsigned>(LoRaProfile::SPREADING_FACTOR),
        static_cast<unsigned>(LoRaProfile::CODING_RATE));
    lv_label_set_text(params, paramsText);
    lv_obj_set_style_text_font(params, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(params, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(params, LV_ALIGN_TOP_LEFT, 0, 28);

    lv_obj_t* packetTitle = lv_label_create(card);
    lv_label_set_text(packetTitle, "Posledni paket:");
    lv_obj_set_style_text_font(packetTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(packetTitle, lv_color_hex(0x56C7FF), 0);
    lv_obj_align(packetTitle, LV_ALIGN_TOP_LEFT, 0, 55);

    packetLabel = lv_label_create(card);
    lv_obj_set_width(packetLabel, 420);
    lv_obj_set_height(packetLabel, 45);
    lv_label_set_long_mode(packetLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(packetLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(packetLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(packetLabel, LV_ALIGN_TOP_LEFT, 0, 76);

    signalLabel = lv_label_create(card);
    lv_obj_set_style_text_font(signalLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(signalLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(signalLabel, LV_ALIGN_TOP_LEFT, 0, 126);

    countersLabel = lv_label_create(card);
    lv_obj_set_style_text_font(countersLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(countersLabel, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(countersLabel, LV_ALIGN_TOP_LEFT, 0, 150);

    messageLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(messageLabel, "OK = testovaci APRS paket | vlevo = menu");
    lv_obj_set_style_text_font(messageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(messageLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(messageLabel, LV_ALIGN_BOTTOM_MID, 0, -72);
}

void update(const Services::RadioService::ViewState& state) {
    if (stateLabel == nullptr || packetLabel == nullptr ||
        signalLabel == nullptr || countersLabel == nullptr) {
        return;
    }

    if (!state.initialized) {
        lv_label_set_text_fmt(stateLabel, "Stav: CHYBA (%d)", static_cast<int>(state.lastError));
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xFF6B6B), 0);
    } else if (state.transmitting) {
        lv_label_set_text(stateLabel, "Stav: TX - vysilani");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xFFB454), 0);
    } else if (state.receiving) {
        lv_label_set_text(stateLabel, "Stav: RX - prijem aktivni");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x42D392), 0);
    } else {
        lv_label_set_text(stateLabel, "Stav: pripravuji radio");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0x92A7C7), 0);
    }

    const char* packetText = state.lastPacketText[0] != '\0'
        ? state.lastPacketText
        : "Cekam na prvni paket...";
    lv_label_set_text(packetLabel, packetText);

    char signalText[128];
    std::snprintf(
        signalText,
        sizeof(signalText),
        "RSSI %.1f dBm | SNR %.1f dB | FErr %.0f Hz%s",
        static_cast<double>(state.lastRssiDbm),
        static_cast<double>(state.lastSnrDb),
        static_cast<double>(state.lastFrequencyErrorHz),
        state.lastPacketHadOeHeader ? " | OE" : "");
    lv_label_set_text(signalLabel, signalText);
    lv_label_set_text_fmt(
        countersLabel,
        "RX %lu | TX %lu | RXERR %lu | chyba %d",
        static_cast<unsigned long>(state.receivedPackets),
        static_cast<unsigned long>(state.transmittedPackets),
        static_cast<unsigned long>(state.receiveErrors),
        static_cast<int>(state.lastError));
}

void setMessage(const char* text) {
    if (messageLabel != nullptr) {
        lv_label_set_text(messageLabel, text);
    }
}

}  // namespace LoRaScreen
}  // namespace Ui
