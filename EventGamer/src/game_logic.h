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

#pragma once

#include "logic.h"

#include <iosfwd>

namespace eventgamer {

class GameProtocol;
class TimeoutManager;
struct OSCMessage;

class GameLogic final : public Logic {
  public:
    GameLogic(GameProtocol &protocol, TimeoutManager &timeout_manager);
    ~GameLogic() override;
    GameLogic(const GameLogic &) = delete;
    GameLogic &operator=(const GameLogic &) = delete;
    void messageReceived(const OSCMessage &message);
    void keyPressed(char key) override;
    void clientAttached(Client &client) override;
    void clientDetaching() override;
    void audioFinished(AudioKey audio) override;
    void setQueuePosition(unsigned int position) override;
    void timeoutExpired() override;

  private:
    // State of the attached player's game. A game only exists on the MadMapper
    // side once the player has sent a direction, so stop is sent and game-over
    // replies are accepted only while running.
    enum class State {
        no_player, // no client attached
        intro,     // game-start announcement playing, directions are ignored
        ready,     // waiting for the first direction to start the game
        running,   // directions are forwarded
        over,      // game-over announcement playing, then hang up
    };

    void dumpMessage(const OSCMessage &message, std::ostream &stream);
    void onGameOver();
    void sendMessage(const OSCMessage &message);
    void sendUp();
    void sendDown();
    void sendLeft();
    void sendRight();
    void sendStop();

    GameProtocol &protocol_;
    TimeoutManager &timeout_manager_;
    Client *client_;

    State state_;
};

} // namespace eventgamer
