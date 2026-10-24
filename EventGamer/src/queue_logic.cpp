#include "queue_logic.h"

namespace eventgamer {

void QueueLogic::keyPressed(Client &, char) {}

void QueueLogic::clientAttached(Client &client) {
    client.playAudio(AudioKey::in_queue);
}

void QueueLogic::clientDetaching(Client &) {}

void QueueLogic::audioFinished(Client &, AudioKey) {}

void QueueLogic::setQueuePosition(Client &, unsigned int) {}

} // namespace eventgamer
