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

#include "config.h"

#include <toml++/toml.hpp>

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <string_view>

namespace eventgamer {
namespace {

const std::map<AudioKey, std::string_view> audio_key_names{
    {AudioKey::in_queue, "in_queue"},
    {AudioKey::game_starts, "game_starts"},
    {AudioKey::game_over, "game_over"},
};

void check_keys(const toml::table& table,
                std::initializer_list<std::string_view> allowed,
                std::string_view section) {
    for (const auto& [key, value] : table) {
        (void)value;
        bool known = false;
        for (const auto candidate : allowed) {
            if (key.str() == candidate) {
                known = true;
                break;
            }
        }
        if (!known) {
            throw std::runtime_error("Unknown configuration key: " +
                (section.empty() ? std::string{} : std::string{section} + ".") +
                std::string{key.str()});
        }
    }
}

const toml::table& require_section(const toml::table& root, std::string_view name) {
    const auto* section = root[name].as_table();
    if (!section) {
        throw std::runtime_error("Required configuration section: " + std::string{name});
    }
    return *section;
}

std::string require_string(const toml::table& table, std::string_view section,
                           std::string_view key) {
    const auto value = table[key].value<std::string>();
    if (!value || value->empty()) {
        throw std::runtime_error(std::string{section} + "." + std::string{key} +
                                 " must be a nonempty string");
    }
    return *value;
}

std::uint16_t require_port(const toml::table& table, std::string_view key) {
    const auto* integer = table[key].as_integer();
    if (!integer || integer->get() < 1 || integer->get() > 65535) {
        throw std::runtime_error("osc." + std::string{key} +
                                 " must be an integer between 1 and 65535");
    }
    return static_cast<std::uint16_t>(integer->get());
}

std::string read_password(const std::filesystem::path& base, const toml::table& sip) {
    const auto relative = std::filesystem::u8path(require_string(sip, "sip", "password_file"));
    if (relative.has_root_path()) {
        throw std::runtime_error("sip.password_file must be a relative path");
    }
    std::ifstream file((base / relative).lexically_normal(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open sip.password_file");
    }
    std::string password;
    std::getline(file, password);
    if (file.bad() || (file.fail() && !file.eof())) {
        throw std::runtime_error("Cannot read sip.password_file");
    }
    // Accept an optional LF or CRLF terminator without trimming password spaces.
    if (!file.eof() && !password.empty() && password.back() == '\r') {
        password.pop_back();
    }
    if (password.empty()) {
        throw std::runtime_error("sip.password_file must contain a nonempty password");
    }
    if (file.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("sip.password_file must contain exactly one line");
    }
    if (file.bad()) {
        throw std::runtime_error("Cannot read sip.password_file");
    }
    return password;
}

} // namespace

Config load_config(const std::filesystem::path& filename) {
    const auto root = toml::parse_file(filename.u8string());
    check_keys(root, {"sip", "audio", "osc"}, "");
    const auto& sip = require_section(root, "sip");
    const auto& audio = require_section(root, "audio");
    const auto& osc = require_section(root, "osc");
    check_keys(sip, {"server", "username", "password_file"}, "sip");
    for (const auto& [key, value] : audio) {
        (void)value;
        const auto known = std::any_of(audio_key_names.begin(), audio_key_names.end(),
            [&](const auto& entry) { return entry.second == key.str(); });
        if (!known) {
            throw std::runtime_error("Unknown configuration key: audio." + std::string{key.str()});
        }
    }
    check_keys(osc, {"address_family", "bind_host", "bind_port", "remote_host", "remote_port"}, "osc");

    const auto family_name = require_string(osc, "osc", "address_family");
    AddressFamily family;
    if (family_name == "ipv4") {
        family = AddressFamily::ipv4;
    } else if (family_name == "ipv6") {
        family = AddressFamily::ipv6;
    } else if (family_name == "auto") {
        family = AddressFamily::automatic;
    } else {
        throw std::runtime_error("osc.address_family must be 'ipv4', 'ipv6', or 'auto'");
    }

    const auto base = std::filesystem::absolute(filename).parent_path();
    AudioConfig audio_config;
    for (const auto& [key, name] : audio_key_names) {
        const auto path = std::filesystem::u8path(require_string(audio, "audio", name));
        audio_config.paths.emplace(key,
            (path.is_absolute() ? path : base / path).lexically_normal());
    }

    return {
        {require_string(sip, "sip", "server"),
         require_string(sip, "sip", "username"),
         read_password(base, sip)},
        audio_config,
        {family, require_string(osc, "osc", "bind_host"), require_port(osc, "bind_port"),
         require_string(osc, "osc", "remote_host"), require_port(osc, "remote_port")}
    };
}

} // namespace eventgamer
