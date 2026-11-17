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

#include <iostream>
#include <stdexcept>

using namespace eventgamer;
using namespace std::chrono_literals;

namespace {

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

// Fails the due attempt at now and checks the delay until the next one, which
// must not be due a moment before that delay has passed.
void failAndExpect(RegistrationRetry &retry, RegistrationRetry::Clock::time_point &now,
                   std::chrono::seconds expected, const char *message) {
    require(retry.failed(now) == expected, message);
    require(!retry.due(now + expected - 1ms), "Retry was due early");
    now += expected;
    require(retry.due(now), "Retry was not due after its delay");
}

} // namespace

int main() {
    try {
        RegistrationRetry retry;
        auto now = RegistrationRetry::Clock::now();

        require(retry.due(now), "First registration was not due at once");
        require(!retry.due(now), "Registration attempt was due twice");

        failAndExpect(retry, now, 5s, "First retry delay is not 5 s");
        failAndExpect(retry, now, 10s, "Retry delay did not double");
        failAndExpect(retry, now, 20s, "Retry delay did not double");
        failAndExpect(retry, now, 40s, "Retry delay did not double");
        failAndExpect(retry, now, 60s, "Retry delay is not capped at 60 s");
        failAndExpect(retry, now, 60s, "Retry delay grew past 60 s");

        // A failure reported twice (an exception and a callback) counts once.
        require(retry.failed(now) == 60s, "Retry delay is not capped at 60 s");
        require(retry.failed(now + 1500ms) == 59s, "Second report changed the schedule");
        require(!retry.due(now + 59s), "Second report moved the retry earlier");
        require(retry.due(now + 60s), "Second report moved the retry later");
        now += 60s;

        retry.succeeded();
        require(!retry.due(now + 1h), "Retry was due after success");
        failAndExpect(retry, now, 5s, "Success did not reset the retry delay");

        // Success while a retry is scheduled cancels it and resets the delay.
        require(retry.failed(now) == 10s, "Retry delay did not double");
        retry.succeeded();
        require(!retry.due(now + 1h), "Success did not cancel the scheduled retry");
        require(retry.failed(now) == 5s, "Success did not reset the retry delay");
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
