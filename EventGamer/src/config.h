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

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>

namespace eventgamer {

enum class AddressFamily { ipv4, ipv6, automatic };

struct SipConfig {
    std::string server;
    std::string username;
    std::string password; // Loaded from password_file during configuration parsing.
};

enum class AudioKey { in_queue, game_starts, game_over };

struct AudioConfig {
    std::map<AudioKey, std::filesystem::path> paths;
};

struct OscConfig {
    AddressFamily address_family;
    std::string bind_host;
    std::uint16_t bind_port;
    std::string remote_host;
    std::uint16_t remote_port;
};

struct Config {
    SipConfig sip;
    AudioConfig audio;
    OscConfig osc;
};

Config load_config(const std::filesystem::path& filename);

} // namespace eventgamer
