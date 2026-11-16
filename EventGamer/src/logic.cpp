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

#include "logic.h"

namespace eventgamer {

Logic::Logic(TimeoutManager &timeout_manager)
    : timeout_manager_(timeout_manager), client_(nullptr) {}

void Logic::clientAttach(Client &client) {
    client_ = &client;
    clientAttached();
}

void Logic::clientDetach() {
    clientDetaching();
    timeout_manager_.cancelTimeout(*this);
    client_ = nullptr;
}

void Logic::hangUp() {
    if (client_)
        client_->hangUp();
}

void Logic::playAudio(AudioKey audio) {
    if (client_)
        client_->playAudio(audio);
}

} // namespace eventgamer
