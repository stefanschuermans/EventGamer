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

#include <iostream>

namespace eventgamer {

// maximum time in second for which there is no input before asking for a key press
static const unsigned int max_idle_seconds = 20;
// maxim number of repeated unanswered key press requests
static const unsigned int max_idle_repeat = 3;

QueueLogic::QueueLogic(TimeoutManager &timeout_manager)
    : timeout_manager_(timeout_manager), client_(nullptr), timeout_cnt_(0), queue_position_(0) {}

QueueLogic::~QueueLogic() { timeout_manager_.cancelTimeout(*this); }

void QueueLogic::keyPressed(char) {
    if (client_)
        client_->playAudio(AudioKey::in_queue);
    timeout_cnt_ = 0;
}

void QueueLogic::clientAttached(Client &client) {
    client_ = &client;
    std::cout << "queue " << queue_position_ << " added" << std::endl;
    timeout_manager_.cancelTimeout(*this);
    client_->playAudio(AudioKey::in_queue);
    timeout_cnt_ = 0;
}

void QueueLogic::clientDetaching() {
    client_ = nullptr;
    std::cout << "queue " << queue_position_ << " leaving" << std::endl;
    timeout_manager_.cancelTimeout(*this);
    timeout_cnt_ = 0;
}

void QueueLogic::audioFinished(AudioKey) {
    timeout_manager_.registerTimeout(*this, max_idle_seconds);
}

void QueueLogic::setQueuePosition(unsigned int position) {
    std::cout << "queue " << queue_position_ << " update to position " << position << std::endl;
    queue_position_ = position;
}

void QueueLogic::timeoutExpired() {
    if (timeout_cnt_ < max_idle_repeat)
        timeout_cnt_++;
    else {
        std::cout << "queue " << queue_position_ << " timeout" << std::endl;
        if (client_)
            client_->hangUp();
    }
    if (client_)
        client_->playAudio(AudioKey::queue_press_key);
}

} // namespace eventgamer
