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

#include "config.h"

#include <cstddef>
#include <functional>
#include <memory>

namespace eventgamer {

// Owns one nonblocking UDP socket. All methods run on the application thread.
class OSCTransport final {
  public:
    using Receiver = std::function<void(const char *, std::size_t)>;
    explicit OSCTransport(const OscConfig &config);
    ~OSCTransport();
    OSCTransport(const OSCTransport &) = delete;
    OSCTransport &operator=(const OSCTransport &) = delete;

    void poll(const Receiver &receiver);
    void send(const char *data, std::size_t size);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace eventgamer
