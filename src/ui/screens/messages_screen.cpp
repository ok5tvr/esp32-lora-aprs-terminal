#include "ui/screens/messages_screen.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "ui/ui_components.h"

namespace Ui {
namespace MessagesScreen {
namespace {

enum class EditorStage : std::uint8_t {
    None,
    Recipient,
    Text
};

App::MessageSendHandler currentSendHandler = nullptr;
void* currentSendContext = nullptr;
Services::MessageStore::ViewState currentState;

lv_obj_t* listObject = nullptr;
lv_obj_t* countLabel = nullptr;
lv_obj_t* statusLabel = nullptr;
lv_obj_t* editorOverlay = nullptr;
lv_obj_t* editorArea = nullptr;
lv_obj_t* editorHint = nullptr;
lv_obj_t* rows[Services::MessageStore::MAX_MESSAGES] = {};

std::uint32_t renderedRevision = 0xFFFFFFFFU;
std::size_t selectedIndex = 0;
char recipientDraft[Aprs::MAX_MESSAGE_ADDRESS_LENGTH + 1] = {};
char textDraft[Aprs::MAX_MESSAGE_TEXT_LENGTH + 1] = {};
EditorStage activeStage = EditorStage::None;
bool pendingOpen = false;
bool pendingReady = false;
bool pendingCancel = false;

void copyText(char* output, std::size_t capacity, const char* input) {
    if (output != nullptr && capacity > 0) {
        std::snprintf(output, capacity, "%s", input != nullptr ? input : "");
    }
}

const char* deliveryText(const Services::MessageStore::Message& message) {
    using Delivery = Services::MessageStore::DeliveryState;
    switch (message.state) {
        case Delivery::Pending: return "CEKA ACK";
        case Delivery::Acknowledged: return "ACK";
        case Delivery::Rejected: return "REJ";
        case Delivery::Failed: return "CHYBA";
        case Delivery::Received: return message.groupMessage ? "SKUPINA" : "PRIJATO";
        default: return "";
    }
}

std::uint32_t deliveryColor(const Services::MessageStore::Message& message) {
    using Delivery = Services::MessageStore::DeliveryState;
    switch (message.state) {
        case Delivery::Acknowledged: return 0x42D392;
        case Delivery::Rejected:
        case Delivery::Failed: return 0xFF6B6B;
        case Delivery::Pending: return 0xFFB454;
        default: return 0x56C7FF;
    }
}

void refreshSelection() {
    for (std::size_t index = 0; index < currentState.count; ++index) {
        if (rows[index] == nullptr) {
            continue;
        }
        lv_obj_set_style_border_color(
            rows[index],
            lv_color_hex(index == selectedIndex ? 0x56C7FF : 0x31425F),
            0);
        lv_obj_set_style_border_width(rows[index], index == selectedIndex ? 2 : 1, 0);
    }
}

void rowClicked(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }
    selectedIndex = static_cast<std::size_t>(
        reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
    refreshSelection();
}

void keyboardEvent(lv_event_t* event) {
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_READY) {
        pendingReady = true;
    } else if (code == LV_EVENT_CANCEL) {
        pendingCancel = true;
    }
}

void closeEditor() {
    if (editorOverlay != nullptr) {
        lv_obj_del(editorOverlay);
    }
    editorOverlay = nullptr;
    editorArea = nullptr;
    editorHint = nullptr;
    activeStage = EditorStage::None;
    pendingReady = false;
    pendingCancel = false;
}

void createEditor(EditorStage stage) {
    activeStage = stage;
    editorOverlay = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(editorOverlay);
    lv_obj_set_size(editorOverlay, 480, 320);
    lv_obj_align(editorOverlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(editorOverlay, lv_color_hex(0x0B1424), 0);
    lv_obj_set_style_bg_opa(editorOverlay, LV_OPA_COVER, 0);
    lv_obj_add_flag(editorOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(editorOverlay);

    lv_obj_t* title = lv_label_create(editorOverlay);
    char titleText[64] = {};
    if (stage == EditorStage::Recipient) {
        std::snprintf(titleText, sizeof(titleText), "Adresat APRS zpravy");
    } else {
        std::snprintf(titleText, sizeof(titleText), "Zprava pro %s", recipientDraft);
    }
    lv_label_set_text(title, titleText);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 14, 7);

    editorArea = lv_textarea_create(editorOverlay);
    lv_obj_set_size(editorArea, 452, stage == EditorStage::Recipient ? 42 : 66);
    lv_obj_align(editorArea, LV_ALIGN_TOP_MID, 0, 37);
    lv_textarea_set_one_line(editorArea, stage == EditorStage::Recipient);
    lv_obj_set_style_text_font(editorArea, &lv_font_montserrat_16, 0);

    if (stage == EditorStage::Recipient) {
        lv_textarea_set_text(editorArea, recipientDraft);
        lv_textarea_set_max_length(editorArea, Aprs::MAX_MESSAGE_ADDRESS_LENGTH);
        lv_textarea_set_accepted_chars(editorArea, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-");
    } else {
        lv_textarea_set_text(editorArea, textDraft);
        lv_textarea_set_max_length(editorArea, Aprs::MAX_MESSAGE_TEXT_LENGTH);
    }

    editorHint = lv_label_create(editorOverlay);
    lv_label_set_text(
        editorHint,
        stage == EditorStage::Recipient
            ? "Potvrdte CALL/SSID tlacitkem klavesnice."
            : "Max. 67 ASCII znaku; znaky | ~ { nejsou povoleny.");
    lv_obj_set_width(editorHint, 452);
    lv_label_set_long_mode(editorHint, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(editorHint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(editorHint, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(editorHint, LV_ALIGN_TOP_LEFT, 14, stage == EditorStage::Recipient ? 84 : 108);

    lv_obj_t* keyboard = lv_keyboard_create(editorOverlay);
    lv_obj_set_size(keyboard, 468, stage == EditorStage::Recipient ? 215 : 190);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_keyboard_set_textarea(keyboard, editorArea);
    lv_keyboard_set_mode(
        keyboard,
        stage == EditorStage::Recipient
            ? LV_KEYBOARD_MODE_TEXT_UPPER
            : LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_event_cb(keyboard, keyboardEvent, LV_EVENT_ALL, nullptr);
    lv_obj_add_state(editorArea, LV_STATE_FOCUSED);
}

void advanceEditor() {
    if (editorArea == nullptr) {
        return;
    }
    const char* value = lv_textarea_get_text(editorArea);
    if (activeStage == EditorStage::Recipient) {
        if (value == nullptr || value[0] == '\0') {
            pendingReady = false;
            if (editorHint != nullptr) {
                lv_label_set_text(editorHint, "Adresat nesmi byt prazdny.");
                lv_obj_set_style_text_color(editorHint, lv_color_hex(0xFF6B6B), 0);
            }
            return;
        }
        copyText(recipientDraft, sizeof(recipientDraft), value);
        lv_obj_del(editorOverlay);
        editorOverlay = nullptr;
        editorArea = nullptr;
        editorHint = nullptr;
        pendingReady = false;
        createEditor(EditorStage::Text);
        return;
    }

    copyText(textDraft, sizeof(textDraft), value);
    if (currentSendHandler == nullptr) {
        setMessage("Odesilani zpravy neni dostupne.");
        closeEditor();
        return;
    }

    char error[128] = {};
    const bool queued = currentSendHandler(
        recipientDraft,
        textDraft,
        error,
        sizeof(error),
        currentSendContext);
    if (queued) {
        closeEditor();
        setMessage("Zprava byla zarazena k odeslani; ceka se na ACK.");
    } else {
        if (editorHint != nullptr) {
            lv_label_set_text(editorHint, error[0] != '\0' ? error : "Zpravu se nepodarilo zaradit.");
            lv_obj_set_style_text_color(editorHint, lv_color_hex(0xFF6B6B), 0);
        }
        pendingReady = false;
    }
}

void createRow(
    const Services::MessageStore::Message& message,
    std::size_t index) {

    lv_obj_t* row = lv_obj_create(listObject);
    rows[index] = row;
    lv_obj_set_size(row, 438, 57);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x17243A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x31425F), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 9, 0);
    lv_obj_set_style_pad_left(row, 10, 0);
    lv_obj_set_style_pad_right(row, 10, 0);
    lv_obj_set_style_pad_top(row, 5, 0);
    lv_obj_set_style_pad_bottom(row, 5, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(
        row,
        rowClicked,
        LV_EVENT_CLICKED,
        reinterpret_cast<void*>(static_cast<std::intptr_t>(index)));

    char titleText[96] = {};
    std::snprintf(
        titleText,
        sizeof(titleText),
        "%c %s%s%s",
        message.direction == Services::MessageStore::Direction::Incoming ? '<' : '>',
        message.peer,
        message.hasMessageId ? "  #" : "",
        message.hasMessageId ? message.messageId : "");
    lv_obj_t* title = lv_label_create(row);
    lv_label_set_text(title, titleText);
    lv_obj_set_width(title, 300);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF4F7FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* state = lv_label_create(row);
    lv_label_set_text(state, deliveryText(message));
    lv_obj_set_style_text_font(state, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(state, lv_color_hex(deliveryColor(message)), 0);
    lv_obj_align(state, LV_ALIGN_TOP_RIGHT, 0, 1);

    lv_obj_t* text = lv_label_create(row);
    lv_label_set_text(text, message.text);
    lv_obj_set_width(text, 408);
    lv_label_set_long_mode(text, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(text, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(text, lv_color_hex(0xBDCAE0), 0);
    lv_obj_align(text, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

}  // namespace

void create(App::MessageSendHandler sendHandler, void* sendContext) {
    currentSendHandler = sendHandler;
    currentSendContext = sendContext;
    currentState = Services::MessageStore::ViewState{};
    selectedIndex = 0;
    renderedRevision = 0xFFFFFFFFU;
    pendingOpen = false;
    pendingReady = false;
    pendingCancel = false;
    editorOverlay = nullptr;
    editorArea = nullptr;
    editorHint = nullptr;
    activeStage = EditorStage::None;
    std::memset(rows, 0, sizeof(rows));

    resetScreen();
    createHeader("APRS zpravy");

    countLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(countLabel, "0/20");
    lv_obj_set_style_text_font(countLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(countLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(countLabel, LV_ALIGN_TOP_RIGHT, -14, 16);

    statusLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(statusLabel, "OK = nova zprava; vyberte radek pro odpoved.");
    lv_obj_set_width(statusLabel, 375);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x92A7C7), 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 14, 34);

    listObject = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(listObject);
    lv_obj_set_size(listObject, 452, 194);
    lv_obj_align(listObject, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_pad_left(listObject, 5, 0);
    lv_obj_set_style_pad_right(listObject, 5, 0);
    lv_obj_set_style_pad_top(listObject, 2, 0);
    lv_obj_set_style_pad_bottom(listObject, 2, 0);
    lv_obj_set_style_pad_row(listObject, 5, 0);
    lv_obj_set_flex_flow(listObject, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        listObject,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(listObject, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(listObject, LV_SCROLLBAR_MODE_AUTO);
}

void update(const Services::MessageStore::ViewState& state) {
    if (listObject == nullptr || countLabel == nullptr ||
        renderedRevision == state.revision) {
        return;
    }

    currentState = state;
    renderedRevision = state.revision;
    if (selectedIndex >= currentState.count) {
        selectedIndex = currentState.count == 0 ? 0 : currentState.count - 1;
    }

    lv_obj_clean(listObject);
    std::memset(rows, 0, sizeof(rows));

    char countText[32] = {};
    std::snprintf(
        countText,
        sizeof(countText),
        "%u/%u P:%u",
        static_cast<unsigned>(state.count),
        static_cast<unsigned>(Services::MessageStore::MAX_MESSAGES),
        static_cast<unsigned>(state.pendingOutgoing));
    lv_label_set_text(countLabel, countText);

    if (state.count == 0) {
        lv_obj_t* empty = lv_label_create(listObject);
        lv_label_set_text(empty, "Zatim nebyla prijata ani odeslana zadna APRS zprava.");
        lv_obj_set_width(empty, 420);
        lv_label_set_long_mode(empty, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(empty, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(empty, lv_color_hex(0x92A7C7), 0);
        return;
    }

    for (std::size_t index = 0; index < state.count; ++index) {
        createRow(state.messages[index], index);
    }
    refreshSelection();
}

void processPending() {
    if (pendingCancel) {
        closeEditor();
        return;
    }
    if (pendingReady) {
        advanceEditor();
        return;
    }
    if (pendingOpen && editorOverlay == nullptr) {
        pendingOpen = false;
        createEditor(EditorStage::Recipient);
    }
}

void scroll(int direction) {
    if (listObject == nullptr || direction == 0) {
        return;
    }
    lv_obj_scroll_by(listObject, 0, direction > 0 ? -62 : 62, LV_ANIM_ON);
}

void compose() {
    if (editorOverlay != nullptr) {
        return;
    }
    recipientDraft[0] = '\0';
    textDraft[0] = '\0';
    if (currentState.count > 0 && selectedIndex < currentState.count) {
        copyText(
            recipientDraft,
            sizeof(recipientDraft),
            currentState.messages[selectedIndex].peer);
    }
    pendingOpen = true;
}

void setMessage(const char* text) {
    if (statusLabel != nullptr) {
        lv_label_set_text(statusLabel, text != nullptr ? text : "");
        lv_obj_set_style_text_color(
            statusLabel,
            (text != nullptr && std::strstr(text, "zarazena") != nullptr)
                ? lv_color_hex(0x42D392)
                : lv_color_hex(0xFFB454),
            0);
    }
}

}  // namespace MessagesScreen
}  // namespace Ui
