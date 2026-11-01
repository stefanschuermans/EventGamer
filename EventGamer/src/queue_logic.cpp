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

#include "queue_logic.h"
#include "timeout_manager.h"

namespace eventgamer {

QueueLogic::QueueLogic(TimeoutManager &timeout_manager)
    : timeout_manager_(timeout_manager), client_(nullptr) {}

QueueLogic::~QueueLogic() { timeout_manager_.cancelTimeout(*this); }

void QueueLogic::keyPressed(char) {}

void QueueLogic::clientAttached(Client &client) {
    client_ = &client;
    timeout_manager_.cancelTimeout(*this);
    client_->playAudio(AudioKey::in_queue);
}

void QueueLogic::clientDetaching() {
    client_ = nullptr;
    timeout_manager_.cancelTimeout(*this);
}

void QueueLogic::audioFinished(AudioKey) {}

void QueueLogic::setQueuePosition(unsigned int) {}

void QueueLogic::timeoutExpired() {}

} // namespace eventgamer
