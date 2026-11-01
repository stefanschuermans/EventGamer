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
#include "timeout_manager.h"

#include <functional>
#include <iostream>
#include <stdexcept>

using namespace eventgamer;

namespace {

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

class TestLogic final : public Logic {
  public:
    void keyPressed(char) override {}
    void clientAttached(Client &) override {}
    void clientDetaching() override {}
    void audioFinished(AudioKey) override {}
    void setQueuePosition(unsigned int) override {}
    void timeoutExpired() override {
        ++calls;
        if (callback)
            callback();
    }

    unsigned int calls = 0;
    std::function<void()> callback;
};

} // namespace

int main() {
    try {
        TimeoutManager manager;
        TestLogic first, second;
        manager.cancelTimeout(first);
        manager.tick();

        manager.registerTimeout(first, 3600);
        manager.tick();
        require(first.calls == 0, "Future timeout fired early");
        manager.registerTimeout(first, 0);
        manager.tick();
        manager.tick();
        require(first.calls == 1, "Replacement timeout must fire exactly once");

        manager.registerTimeout(first, 0);
        manager.registerTimeout(first, 3600);
        manager.tick();
        require(first.calls == 1, "Replaced immediate timeout still fired");
        manager.cancelTimeout(first);
        manager.cancelTimeout(first);

        manager.registerTimeout(first, 0);
        manager.cancelTimeout(first);
        manager.tick();
        require(first.calls == 1, "Cancelled timeout fired");

        first.callback = [&] {
            manager.cancelTimeout(second);
            manager.registerTimeout(first, 3600);
        };
        manager.registerTimeout(first, 0);
        manager.registerTimeout(second, 0);
        manager.tick();
        require(first.calls == 2 && second.calls == 0,
                "Callback cancellation or rescheduling failed");
        manager.tick();
        require(first.calls == 2, "Rescheduled timeout fired early");
        manager.cancelTimeout(first);

        first.callback = {};
        manager.registerTimeout(first, 0);
        manager.registerTimeout(second, 0);
        manager.tick();
        require(first.calls == 3 && second.calls == 1, "Due timeouts were not drained");
        manager.tick();
        require(first.calls == 3 && second.calls == 1, "Timeout fired more than once");
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
