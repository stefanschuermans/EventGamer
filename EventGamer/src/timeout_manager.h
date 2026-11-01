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

#include <chrono>
#include <map>
#include <vector>

namespace eventgamer {

class Logic;

// Used on the same thread as the logic callbacks. Registered logic objects must
// remain alive until their timeout expires or is cancelled.
class TimeoutManager {
  public:
    void registerTimeout(Logic &logic, unsigned int seconds);
    void cancelTimeout(Logic &logic);
    void tick();

  private:
    using TimePoint = std::chrono::steady_clock::time_point;

    std::map<Logic *, TimePoint> logic_timeouts_;
    std::map<TimePoint, std::vector<Logic *>> timeout_logics_;
};

} // namespace eventgamer
