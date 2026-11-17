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
#include <optional>

namespace eventgamer {

// Paces SIP registration attempts, so registration never gives up. The first
// attempt is due at once. After each failure the delay doubles from 5 s up to
// 60 s; a success resets it. Used on the main thread only.
class RegistrationRetry {
  public:
    using Clock = std::chrono::steady_clock;

    RegistrationRetry();

    // True once when the scheduled attempt is due; the schedule is then cleared
    // until the attempt fails.
    bool due(Clock::time_point now);
    // Schedules the next attempt after a failed one and returns the time until
    // it. A failure reported while an attempt is already scheduled keeps that
    // schedule, so one failure reported twice counts once.
    std::chrono::seconds failed(Clock::time_point now);
    void succeeded();

  private:
    std::optional<Clock::time_point> next_attempt_;
    std::chrono::seconds next_delay_;
};

} // namespace eventgamer
