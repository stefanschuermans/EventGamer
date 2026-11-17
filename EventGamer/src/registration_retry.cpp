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

#include "registration_retry.h"

#include <algorithm>

namespace eventgamer {

// Retry quickly after a single failure, and at most once a minute during a
// longer outage, so the installation is reachable again soon after it ends.
static const std::chrono::seconds first_retry_delay(5);
static const std::chrono::seconds max_retry_delay(60);

RegistrationRetry::RegistrationRetry()
    : next_attempt_(Clock::time_point::min()), next_delay_(first_retry_delay) {}

bool RegistrationRetry::due(Clock::time_point now) {
    if (!next_attempt_ || now < *next_attempt_)
        return false;
    next_attempt_.reset();
    return true;
}

std::chrono::seconds RegistrationRetry::failed(Clock::time_point now) {
    if (next_attempt_) {
        if (*next_attempt_ <= now)
            return std::chrono::seconds::zero();
        return std::chrono::ceil<std::chrono::seconds>(*next_attempt_ - now);
    }
    const auto delay = next_delay_;
    next_attempt_ = now + delay;
    next_delay_ = std::min(next_delay_ * 2, max_retry_delay);
    return delay;
}

void RegistrationRetry::succeeded() {
    next_attempt_.reset();
    next_delay_ = first_retry_delay;
}

} // namespace eventgamer
