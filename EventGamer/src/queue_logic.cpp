#include "queue_logic.h"

namespace eventgamer {

QueueLogic::QueueLogic() : client_(nullptr) {}

void QueueLogic::keyPressed(char) {}

void QueueLogic::clientAttached(Client &client) {
    client_ = &client;
    client_->playAudio(AudioKey::in_queue);
}

void QueueLogic::clientDetaching() { client_ = nullptr; }

void QueueLogic::audioFinished(AudioKey) {}

void QueueLogic::setQueuePosition(unsigned int) {}

} // namespace eventgamer
