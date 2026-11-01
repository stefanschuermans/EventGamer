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

#include "timeout_manager.h"

#include "logic.h"

#include <algorithm>

namespace eventgamer {

void TimeoutManager::registerTimeout(Logic &logic, unsigned int seconds) {
    cancelTimeout(logic);
    const auto timestamp = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    logic_timeouts_[&logic] = timestamp;
    timeout_logics_[timestamp].push_back(&logic);
}

void TimeoutManager::cancelTimeout(Logic &logic) {
    const auto registration = logic_timeouts_.find(&logic);
    if (registration == logic_timeouts_.end())
        return;

    const auto timeout = timeout_logics_.find(registration->second);
    auto &logics = timeout->second;
    logics.erase(std::remove(logics.begin(), logics.end(), &logic), logics.end());
    if (logics.empty())
        timeout_logics_.erase(timeout);
    logic_timeouts_.erase(registration);
}

void TimeoutManager::tick() {
    const auto now = std::chrono::steady_clock::now();
    while (!timeout_logics_.empty() && timeout_logics_.begin()->first <= now) {
        Logic *logic = timeout_logics_.begin()->second.front();
        // Re-read the maps after each callback: it may cancel or replace any
        // registration, including another logic's timeout at the same timestamp.
        cancelTimeout(*logic);
        logic->timeoutExpired();
    }
}

} // namespace eventgamer
