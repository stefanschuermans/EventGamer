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

#include <vector>

namespace eventgamer {

class TimeoutManager;

class QueueLogic final : public Logic {
  public:
    explicit QueueLogic(TimeoutManager &timeout_manager);
    ~QueueLogic() override;
    QueueLogic(const QueueLogic &) = delete;
    QueueLogic &operator=(const QueueLogic &) = delete;
    void keyPressed(char key) override;
    void clientAttached(Client &client) override;
    void clientDetaching() override;
    void audioFinished(AudioKey audio) override;
    void setQueuePosition(unsigned int position) override;
    void timeoutExpired() override;

  private:
    void startPlayPosition();

    TimeoutManager &timeout_manager_;
    Client *client_;
    unsigned int timeout_cnt_;
    unsigned int queue_position_;
    /** part of digits of current position in queue that still need to be played
        to SIP, store in reverse order for algo simplicity */
    std::vector<unsigned char> play_position_reverse_;
};

} // namespace eventgamer
