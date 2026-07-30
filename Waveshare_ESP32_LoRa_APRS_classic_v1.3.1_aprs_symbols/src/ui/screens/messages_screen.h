#pragma once

#include "app/app_types.h"
#include "services/message_store.h"

namespace Ui {
namespace MessagesScreen {

void create(App::MessageSendHandler sendHandler, void* sendContext);
void update(const Services::MessageStore::ViewState& state);
void processPending();
void scroll(int direction);
void compose();
void setMessage(const char* text);

}  // namespace MessagesScreen
}  // namespace Ui
