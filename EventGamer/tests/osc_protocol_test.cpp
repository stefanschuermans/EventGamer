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

#include <asio.hpp>
#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>

#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace eventgamer;

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

int main(int argc, char **argv) {
    const bool ipv6 = argc > 1 && std::string(argv[1]) == "ipv6";
    try {
        asio::io_context context;
        const auto host = ipv6 ? "::1" : "127.0.0.1";
        asio::ip::udp::socket peer(context);
        asio::error_code error;
        peer.open(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4(), error);
        if (error && ipv6)
            return 77;
        if (error)
            throw asio::system_error(error);
        peer.bind({asio::ip::make_address(host), 0}, error);
        if (error && ipv6)
            return 77;
        if (error)
            throw asio::system_error(error);
        peer.non_blocking(true);
        const auto family = argc > 1 && std::string(argv[1]) == "auto" ? AddressFamily::automatic
                            : ipv6                                     ? AddressFamily::ipv6
                                                                       : AddressFamily::ipv4;
        OSCTransport transport({family, host, 0, host, peer.local_endpoint().port()});
        GameProtocol protocol(transport);
        const OSCMessage outgoing{"/test/state",
                                  {std::int32_t{42}, 1.25f, std::string{"ready"}, true, false,
                                   std::int64_t{1234567890123LL}, 2.5}};
        protocol.sendMessage(outgoing);

        std::array<char, 65536> buffer;
        asio::ip::udp::endpoint source;
        std::size_t size = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        do {
            size = peer.receive_from(asio::buffer(buffer), source, 0, error);
            if (!error)
                break;
            require(error == asio::error::would_block || error == asio::error::try_again,
                    "UDP receive failed");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } while (std::chrono::steady_clock::now() < deadline);
        require(!error, "Outgoing message timed out");
        osc::ReceivedPacket packet(buffer.data(), static_cast<int>(size));
        osc::ReceivedMessage decoded(packet);
        require(std::string(decoded.AddressPattern()) == outgoing.address,
                "Wrong outgoing address");
        require(decoded.ArgumentCount() == outgoing.arguments.size(), "Wrong argument count");

        unsigned received = 0;
        protocol.setHandler(outgoing.address, [&](const OSCMessage &message) {
            require(message.arguments == outgoing.arguments, "Argument round trip failed");
            ++received;
        });
        // A malformed datagram must not prevent processing the next valid one.
        peer.send_to(asio::buffer("bad", 3), source);
        peer.send_to(asio::buffer(buffer.data(), size), source);
        auto wait_for = [&](unsigned count) {
            const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            while (received < count && std::chrono::steady_clock::now() < end) {
                protocol.poll();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            require(received == count, "Incoming message dispatch failed");
        };
        wait_for(1);

        std::array<char, 1024> bundle_buffer;
        osc::OutboundPacketStream bundle(bundle_buffer.data(), bundle_buffer.size());
        bundle << osc::BeginBundleImmediate << osc::BeginMessage("/test/bundle") << osc::EndMessage
               << osc::EndBundle;
        protocol.setHandler("/test/bundle", [&](const OSCMessage &) { ++received; });
        peer.send_to(asio::buffer(bundle.Data(), bundle.Size()), source);
        wait_for(2);
        bool rejected = false;
        try {
            protocol.sendMessage({"invalid", {}});
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        require(rejected, "Invalid outgoing address was accepted");
        std::cout << "OSC message, bundle, and malformed-packet checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
