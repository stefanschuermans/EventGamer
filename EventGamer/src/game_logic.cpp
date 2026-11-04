/*
 * EventGamer - telephone controls for Snake in MadMapper.
 * Copyright (C) 2026 Stefan Schuermans
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "game_logic.h"
#include "game_protocol.h"
#include "timeout_manager.h"

#include <iomanip>
#include <iostream>
#include <type_traits>

namespace eventgamer {

// time out the game after not input for some time
static const unsigned int no_input_timeout_seconds = 20;

GameLogic::GameLogic(GameProtocol &protocol, TimeoutManager &timeout_manager)
    : protocol_(protocol), timeout_manager_(timeout_manager), client_(nullptr),
      state_(State::no_player) {
    protocol_.setHandler([this](const OSCMessage &message) { messageReceived(message); });
}

GameLogic::~GameLogic() {
    timeout_manager_.cancelTimeout(*this);
    protocol_.setHandler(GameProtocol::Handler{});
}

void GameLogic::messageReceived(const OSCMessage &message) {
    std::cout << "Game OSC message received: ";
    dumpMessage(message, std::cout);
    std::cout << std::endl;
    if (message.address == "/snake_reply/game_over") {
        onGameOver();
    }
}

void GameLogic::keyPressed(char key) {
    std::cout << "game key " << (key <= ' ' ? '?' : key) << std::endl;
    timeout_manager_.registerTimeout(*this, no_input_timeout_seconds);
    if (key != '2' && key != '4' && key != '6' && key != '8')
        return;
    switch (state_) {
    case State::ready:
        // MadMapper starts a new game on the first direction.
        std::cout << "game started by first direction" << std::endl;
        state_ = State::running;
        break;
    case State::running:
        break;
    case State::intro:
        std::cout << "game key ignored: announcement still playing" << std::endl;
        return;
    case State::over:
    case State::no_player:
        std::cout << "game key ignored: no running game" << std::endl;
        return;
    }
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
    state_ = State::intro;
    std::cout << "game client joining" << std::endl;
    timeout_manager_.registerTimeout(*this, no_input_timeout_seconds);
    // No restart: the game starts with the player's first direction, which also
    // gives MadMapper time to finish a previous game's death and score screens.
    client_->playAudio(AudioKey::game_starts);
}

void GameLogic::clientDetaching() {
    std::cout << "game client leaving" << std::endl;
    const bool running = state_ == State::running;
    // Forget the client before sending, so a failing send cannot leave a
    // reference to a call that is about to be destroyed.
    state_ = State::no_player;
    client_ = nullptr;
    timeout_manager_.cancelTimeout(*this);
    if (running)
        sendStop();
}

void GameLogic::audioFinished(AudioKey audio) {
    if (state_ == State::intro && audio == AudioKey::game_starts) {
        std::cout << "game ready, waiting for first direction" << std::endl;
        state_ = State::ready;
    } else if (state_ == State::over && audio == AudioKey::game_over && client_) {
        client_->hangUp();
    }
}

void GameLogic::setQueuePosition(unsigned int) {}

void GameLogic::timeoutExpired() {
    std::cout << "game timeout" << std::endl;
    if (client_)
        client_->hangUp();
}

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
    // Only the current player's running game can end. Repeated replies and
    // replies to a previous player's stop arrive while not running.
    if (state_ != State::running) {
        std::cout << "game over ignored: no running game" << std::endl;
        return;
    }
    std::cout << "game over" << std::endl;
    state_ = State::over;
    if (client_)
        client_->playAudio(AudioKey::game_over);
}

void GameLogic::sendMessage(const OSCMessage &message) {
    std::cout << "Sending game OSC message: ";
    dumpMessage(message, std::cout);
    std::cout << std::endl;
    // UDP gives no delivery guarantee anyway. A failed send is logged, so it
    // cannot end the program from a key press or timeout callback.
    try {
        protocol_.sendMessage(message);
    } catch (const std::exception &error) {
        std::cerr << "Cannot send game OSC message: " << error.what() << std::endl;
    }
}

void GameLogic::sendUp() { sendMessage({"/snake/up", {1.0f}}); }

void GameLogic::sendDown() { sendMessage({"/snake/down", {1.0f}}); }

void GameLogic::sendLeft() { sendMessage({"/snake/left", {1.0f}}); }

void GameLogic::sendRight() { sendMessage({"/snake/right", {1.0f}}); }

void GameLogic::sendStop() { sendMessage({"/snake/stop", {1.0f}}); }

} // namespace eventgamer
