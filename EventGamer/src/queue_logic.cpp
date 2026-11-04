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
#include "config.h"
#include "timeout_manager.h"

#include <iostream>
#include <vector>

namespace eventgamer {

// maximum time in second for which there is no input before asking for a key press
static const unsigned int max_idle_seconds = 20;
// maxim number of repeated unanswered key press requests
static const unsigned int max_idle_repeat = 3;

QueueLogic::QueueLogic(TimeoutManager &timeout_manager)
    : timeout_manager_(timeout_manager), client_(nullptr), timeout_cnt_(0), queue_position_(0),
      play_position_reverse_() {}

QueueLogic::~QueueLogic() { timeout_manager_.cancelTimeout(*this); }

void QueueLogic::keyPressed(char) {
    timeout_cnt_ = 0;
    startPlayPosition();
}

void QueueLogic::clientAttached(Client &client) {
    client_ = &client;
    std::cout << "queue " << queue_position_ << " added" << std::endl;
    timeout_cnt_ = 0;
    startPlayPosition();
}

void QueueLogic::clientDetaching() {
    client_ = nullptr;
    std::cout << "queue " << queue_position_ << " leaving" << std::endl;
    timeout_manager_.cancelTimeout(*this);
    timeout_cnt_ = 0;
}

void QueueLogic::audioFinished(AudioKey) {
    // no more digits of position to play -> start idle phase
    if (play_position_reverse_.empty()) {
        timeout_manager_.registerTimeout(*this, max_idle_seconds);
        return;
    }

    // play next digit
    unsigned char digit = play_position_reverse_.back();
    play_position_reverse_.pop_back();
    static const AudioKey digit_audio_keys[] = {
        AudioKey::digit_0, AudioKey::digit_1, AudioKey::digit_2, AudioKey::digit_3,
        AudioKey::digit_4, AudioKey::digit_5, AudioKey::digit_6, AudioKey::digit_7,
        AudioKey::digit_8, AudioKey::digit_9,
    };
    static const unsigned int digit_audio_key_cnt =
        sizeof(digit_audio_keys) / sizeof(digit_audio_keys[0]);
    if (client_)
        client_->playAudio(digit_audio_keys[digit % digit_audio_key_cnt]);
}

void QueueLogic::setQueuePosition(unsigned int position) {
    // Callers joining or leaving behind this caller do not change its position:
    // keep the current announcement and idle timer running.
    if (position == queue_position_)
        return;
    std::cout << "queue " << queue_position_ << " update to position " << position << std::endl;
    queue_position_ = position;
    startPlayPosition();
}

void QueueLogic::timeoutExpired() {
    if (timeout_cnt_ < max_idle_repeat)
        timeout_cnt_++;
    else {
        std::cout << "queue " << queue_position_ << " timeout" << std::endl;
        // Hanging up detaches this logic from the call, which usually destroys
        // it: return without touching any member afterwards.
        if (client_)
            client_->hangUp();
        return;
    }
    if (client_)
        client_->playAudio(AudioKey::queue_press_key);
    play_position_reverse_.clear(); // don't play position after "press key"
}

void QueueLogic::startPlayPosition() {
    timeout_manager_.cancelTimeout(*this);
    if (client_)
        client_->playAudio(AudioKey::in_queue);

    // split current queue position into digits still to play, stored in
    // reverse, never report position 0 to user
    unsigned int position = queue_position_ == 0 ? 1 : queue_position_;
    play_position_reverse_.clear();
    while (position != 0) {
        play_position_reverse_.push_back(position % 10);
        position /= 10;
    }
}

} // namespace eventgamer
