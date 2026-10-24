#include "game_logic.h"
#include "game_protocol.h"

#include <iomanip>
#include <iostream>
#include <type_traits>

namespace eventgamer {

GameLogic::GameLogic(GameProtocol &protocol)
    : protocol_(protocol), client_(nullptr), game_over_(false) {
    protocol_.setHandler([this](const OSCMessage &message) { messageReceived(message); });
}

GameLogic::~GameLogic() { protocol_.setHandler(GameProtocol::Handler{}); }

void GameLogic::messageReceived(const OSCMessage &message) {
    std::cout << "Game OSC message received: ";
    dumpMessage(message, std::cout);
    std::cout << std::endl;
    if (message.address == "/snake_reply/game_over") {
        onGameOver();
    }
}

void GameLogic::keyPressed(char key) {
    if (game_over_)
        return;
    switch (key) {
    case '2':
        sendUp();
        break;
    case '4':
        sendLeft();
        break;
    case '6':
        sendRight();
        break;
    case '8':
        sendDown();
        break;
    }
}

void GameLogic::clientAttached(Client &client) {
    client_ = &client;
    client_->playAudio(AudioKey::game_starts);
    sendRestart();
}

void GameLogic::clientDetaching() {
    sendStop();
    client_ = nullptr;
}

void GameLogic::audioFinished(AudioKey) {
    if (game_over_ && client_)
        client_->hangUp();
}

void GameLogic::setQueuePosition(unsigned int) {}

void GameLogic::dumpMessage(const OSCMessage &message, std::ostream &stream) {
    stream << message.address;
    for (const auto &argument : message.arguments) {
        stream << ' ';
        std::visit(
            [&stream](const auto &value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    stream << std::quoted(value);
                } else if constexpr (std::is_same_v<T, bool>) {
                    stream << (value ? "true" : "false");
                } else {
                    stream << value;
                }
            },
            argument);
    }
    stream << '\n';
}

void GameLogic::onGameOver() {
    game_over_ = true;
    if (client_)
        client_->playAudio(AudioKey::game_over);
}

void GameLogic::sendMessage(const OSCMessage &message) {
    std::cout << "Sending game OSC message: ";
    dumpMessage(message, std::cout);
    std::cout << std::endl;
    protocol_.sendMessage(message);
}

void GameLogic::sendUp() { sendMessage({"/snake/up", {1.0f}}); }

void GameLogic::sendDown() { sendMessage({"/snake/down", {1.0f}}); }

void GameLogic::sendLeft() { sendMessage({"/snake/left", {1.0f}}); }

void GameLogic::sendRight() { sendMessage({"/snake/right", {1.0f}}); }

void GameLogic::sendRestart() { sendMessage({"/snake/restart", {1.0f}}); }

void GameLogic::sendStop() { sendMessage({"/snake/stop", {1.0f}}); }

} // namespace eventgamer
