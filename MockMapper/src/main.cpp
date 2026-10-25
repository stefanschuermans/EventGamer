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

#include "osc_transport.h"
#include "shutdown.h"

#include <osc/OscOutboundPacketStream.h>
#include <osc/OscPrintReceivedElements.h>
#include <osc/OscReceivedElements.h>

#include <array>
#include <charconv>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace {

std::uint16_t parse_port(std::string_view text) {
    unsigned value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value == 0 ||
        value > 65535) {
        throw std::invalid_argument("Ports must be integers between 1 and 65535");
    }
    return static_cast<std::uint16_t>(value);
}

class MockMapper final {
  public:
    explicit MockMapper(eventgamer::OSCTransport &transport) : transport_(transport) {}

    void receive(const char *data, std::size_t size, unsigned depth = 0) {
        if (depth > 16)
            throw std::runtime_error("OSC bundle nesting exceeds 16 levels");
        osc::ReceivedPacket packet(data, static_cast<osc::osc_bundle_element_size_t>(size));
        if (packet.IsBundle()) {
            osc::ReceivedBundle bundle(packet);
            if (bundle.TimeTag() != 1)
                throw std::runtime_error("Only immediate bundles are supported");
            for (auto element = bundle.ElementsBegin(); element != bundle.ElementsEnd();
                 ++element) {
                receive(element->Contents(), element->Size(), depth + 1);
            }
            return;
        }
        osc::ReceivedMessage message(packet);
        std::cout << "Received: " << message << std::endl;
        const std::string identity(data, size);
        if (identity == previous_) {
            ++consecutive_;
        } else {
            previous_ = identity;
            consecutive_ = 1;
        }
        if (consecutive_ == 3) {
            consecutive_ = 0;
            std::array<char, 128> buffer;
            osc::OutboundPacketStream reply(buffer.data(), buffer.size());
            reply << osc::BeginMessage("/snake_reply/game_over") << 1.0f << osc::EndMessage;
            osc::ReceivedPacket reply_packet(reply.Data(), static_cast<int>(reply.Size()));
            std::cout << "Sending: " << osc::ReceivedMessage(reply_packet) << std::endl;
            transport_.send(reply.Data(), reply.Size());
        }
    }

  private:
    eventgamer::OSCTransport &transport_;
    std::string previous_;
    unsigned consecutive_ = 0;
};

constexpr auto usage =
    "Usage: MockMapper <ipv4/ipv6> <bind_host> <bind_port> <remote_host> <remote_port>\n";

} // namespace

int main(int argc, char **argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        std::cout << usage;
        return 0;
    }
    if (argc != 6) {
        std::cerr << usage;
        return 1;
    }
    try {
        const std::string_view family(argv[1]);
        if (family != "ipv4" && family != "ipv6") {
            throw std::invalid_argument("Address family must be ipv4 or ipv6");
        }
        eventgamer::install_shutdown_handlers();
        std::cout
            << "Dumping OSC messages. Send same message three times to trigger game_over reply."
            << std::endl;
        eventgamer::OSCTransport transport(
            {family == "ipv4" ? eventgamer::AddressFamily::ipv4 : eventgamer::AddressFamily::ipv6,
             argv[2], parse_port(argv[3]), argv[4], parse_port(argv[5])});
        MockMapper mock(transport);
        while (!eventgamer::shutdown_requested()) {
            transport.poll([&](const char *data, std::size_t size) { mock.receive(data, size); });
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "MockMapper error: " << error.what() << '\n';
        return 1;
    }
}
