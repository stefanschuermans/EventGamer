#include "game_logic.h"

namespace eventgamer {

void GameLogic::keyPressed(Client &, char) {}

void GameLogic::clientAttached(Client &client) {
    client.playAudio(AudioKey::game_starts);
}

void GameLogic::clientDetaching(Client &) {}

void GameLogic::audioFinished(Client &, AudioKey) {}

void GameLogic::setQueuePosition(Client &, unsigned int) {}

} // namespace eventgamer
