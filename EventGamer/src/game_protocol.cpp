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

#include "game_protocol.h"

#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>

#include <array>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace eventgamer {
namespace {
void validate_address(const std::string &address) {
    if (address.empty() || address.front() != '/' || address.find('\0') != std::string::npos) {
        throw std::invalid_argument("OSC address must start with '/' and contain no NUL bytes");
    }
}
} // namespace

GameProtocol::GameProtocol(OSCTransport &transport) : transport_(transport) {}

void GameProtocol::poll() {
    transport_.poll([this](const char *data, std::size_t size) { receivePacket(data, size); });
}

void GameProtocol::setHandler(const std::string &address, Handler handler) {
    validate_address(address);
    if (handler) {
        handlers_.insert_or_assign(address, std::move(handler));
    } else {
        handlers_.erase(address);
    }
}

void GameProtocol::setHandler(Handler handler) { default_handler_ = std::move(handler); }

void GameProtocol::sendMessage(const OSCMessage &message) {
    validate_address(message.address);
    std::array<char, 65504> buffer;
    osc::OutboundPacketStream packet(buffer.data(), buffer.size());
    packet << osc::BeginMessage(message.address.c_str());
    for (const auto &argument : message.arguments) {
        std::visit(
            [&](const auto &value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    if (value.find('\0') != std::string::npos) {
                        throw std::invalid_argument("OSC strings cannot contain NUL bytes");
                    }
                    packet << value.c_str();
                } else if constexpr (std::is_same_v<T, std::int64_t>) {
                    packet << static_cast<osc::int64>(value);
                } else if constexpr (std::is_same_v<T, std::int32_t>) {
                    packet << static_cast<osc::int32>(value);
                } else {
                    packet << value;
                }
            },
            argument);
    }
    packet << osc::EndMessage;
    transport_.send(packet.Data(), packet.Size());
}

void GameProtocol::receivePacket(const char *data, std::size_t size, unsigned depth) {
    if (depth > 16) {
        throw std::runtime_error("OSC bundle nesting exceeds 16 levels");
    }
    osc::ReceivedPacket packet(data, static_cast<osc::osc_bundle_element_size_t>(size));
    if (packet.IsBundle()) {
        osc::ReceivedBundle bundle(packet);
        if (bundle.TimeTag() != 1) {
            throw std::runtime_error("Only immediate OSC bundles are supported");
        }
        for (auto element = bundle.ElementsBegin(); element != bundle.ElementsEnd(); ++element) {
            receivePacket(element->Contents(), element->Size(), depth + 1);
        }
        return;
    }
    osc::ReceivedMessage incoming(packet);
    OSCMessage message{incoming.AddressPattern(), {}};
    validate_address(message.address);
    // Game-over is a notification: its payload is intentionally ignored, including
    // OSC argument types that the typed message API does not otherwise expose.
    if (message.address != "/snake_reply/game_over") {
        for (auto argument = incoming.ArgumentsBegin(); argument != incoming.ArgumentsEnd();
             ++argument) {
            if (argument->IsInt32())
                message.arguments.emplace_back(static_cast<std::int32_t>(argument->AsInt32()));
            else if (argument->IsFloat())
                message.arguments.emplace_back(argument->AsFloat());
            else if (argument->IsString())
                message.arguments.emplace_back(std::string(argument->AsString()));
            else if (argument->IsBool())
                message.arguments.emplace_back(argument->AsBool());
            else if (argument->IsInt64())
                message.arguments.emplace_back(static_cast<std::int64_t>(argument->AsInt64()));
            else if (argument->IsDouble())
                message.arguments.emplace_back(argument->AsDouble());
            else
                throw std::runtime_error("Unsupported OSC argument type");
        }
    }
    const auto found = handlers_.find(message.address);
    if (found != handlers_.end()) {
        auto handler = found->second; // Handler may change registrations while executing.
        handler(message);
    } else if (default_handler_) {
        auto handler = default_handler_;
        handler(message);
    } else {
        std::cout << "Unhandled OSC message: " << message.address << std::endl;
    }
}

} // namespace eventgamer
