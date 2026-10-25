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

#include "osc_transport.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace eventgamer {

using OSCArgument = std::variant<std::int32_t, float, std::string, bool, std::int64_t, double>;
struct OSCMessage {
    std::string address;
    std::vector<OSCArgument> arguments;
};

class GameProtocol final {
  public:
    using Handler = std::function<void(const OSCMessage &)>;
    explicit GameProtocol(OSCTransport &transport);
    void poll();
    void sendMessage(const OSCMessage &message);
    // Exact OSC address matching; replace a handler by registering it again.
    void setHandler(const std::string &address, Handler handler);
    // Receives messages without an address-specific handler. Empty clears it.
    void setHandler(Handler handler);

  private:
    void receivePacket(const char *data, std::size_t size, unsigned depth = 0);
    OSCTransport &transport_;
    std::map<std::string, Handler> handlers_;
    Handler default_handler_;
};

} // namespace eventgamer
