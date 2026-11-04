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

#include "game_logic.h"
#include "game_protocol.h"
#include "timeout_manager.h"

#include <asio.hpp>
#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>

#include <array>
#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace eventgamer;

namespace {

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

// Stands in for ActiveCall: hanging up detaches the logic, like setLogic(nullptr).
class FakeClient final : public Client {
  public:
    void playAudio(AudioKey audio) override { played.push_back(audio); }
    void hangUp() override {
        ++hangups;
        if (auto *current = logic) {
            logic = nullptr;
            current->clientDetaching();
        }
    }

    Logic *logic = nullptr;
    std::vector<AudioKey> played;
    unsigned int hangups = 0;
};

// Plays the MadMapper side of the OSC connection on a loopback socket.
class FakeMapper {
  public:
    FakeMapper() : socket_(context_) {
        socket_.open(asio::ip::udp::v4());
        socket_.bind({asio::ip::make_address("127.0.0.1"), 0});
        socket_.non_blocking(true);
    }

    std::uint16_t port() const { return socket_.local_endpoint().port(); }

    // Next received OSC address, or nothing if no datagram arrives in time.
    std::optional<std::string> receive(std::chrono::milliseconds timeout) {
        const auto end = std::chrono::steady_clock::now() + timeout;
        do {
            asio::error_code error;
            const auto size = socket_.receive_from(asio::buffer(buffer_), eventgamer_, 0, error);
            if (!error) {
                osc::ReceivedPacket packet(buffer_.data(),
                                           static_cast<osc::osc_bundle_element_size_t>(size));
                return std::string(osc::ReceivedMessage(packet).AddressPattern());
            }
            require(error == asio::error::would_block || error == asio::error::try_again,
                    "UDP receive failed");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } while (std::chrono::steady_clock::now() < end);
        return std::nullopt;
    }

    void expect(const std::string &address) {
        const auto received = receive(std::chrono::seconds(2));
        require(received.has_value(), "Expected OSC message was not sent");
        if (*received != address) {
            throw std::runtime_error("Expected " + address + " but received " + *received);
        }
    }

    void expectNothing() {
        const auto received = receive(std::chrono::milliseconds(50));
        if (received) {
            throw std::runtime_error("Unexpected OSC message " + *received);
        }
    }

    // Requires a message from EventGamer first, so its address is known.
    void sendGameOver(GameProtocol &protocol) {
        std::array<char, 256> data;
        osc::OutboundPacketStream packet(data.data(), data.size());
        packet << osc::BeginMessage("/snake_reply/game_over") << 1.0f << osc::EndMessage;
        socket_.send_to(asio::buffer(packet.Data(), packet.Size()), eventgamer_);
        const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
        while (std::chrono::steady_clock::now() < end) {
            protocol.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

  private:
    asio::io_context context_;
    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint eventgamer_;
    std::array<char, 65536> buffer_;
};

void attach(GameLogic &game, FakeClient &client) {
    client.logic = &game;
    game.clientAttached(client);
}

void detach(FakeClient &client) {
    if (auto *current = client.logic) {
        client.logic = nullptr;
        current->clientDetaching();
    }
}

} // namespace

int main() {
    try {
        FakeMapper mapper;
        OSCTransport transport({AddressFamily::ipv4, "127.0.0.1", 0, "127.0.0.1", mapper.port()});
        GameProtocol protocol(transport);
        TimeoutManager timeout_manager;
        GameLogic game(protocol, timeout_manager);
        // Let the fake mapper learn EventGamer's address for its replies.
        protocol.sendMessage({"/test/probe", {}});
        mapper.expect("/test/probe");

        {
            // No restart on attach; directions wait for the announcement.
            FakeClient player;
            attach(game, player);
            mapper.expectNothing();
            require(player.played == std::vector<AudioKey>{AudioKey::game_starts},
                    "Game start announcement not played");
            game.keyPressed('2');
            mapper.expectNothing();

            game.audioFinished(AudioKey::game_starts);
            game.keyPressed('5');
            mapper.expectNothing();
            game.keyPressed('2');
            mapper.expect("/snake/up");
            game.keyPressed('4');
            mapper.expect("/snake/left");

            // Leaving a running game (e.g. remote hang-up) stops it.
            detach(player);
            mapper.expect("/snake/stop");
            require(player.hangups == 0, "Detaching must not hang up");
        }

        {
            // A player who never started a game leaves without a stop.
            FakeClient player;
            attach(game, player);
            game.audioFinished(AudioKey::game_starts);
            detach(player);
            mapper.expectNothing();
        }

        {
            // Game over is accepted once; duplicates are ignored.
            FakeClient player;
            attach(game, player);
            game.audioFinished(AudioKey::game_starts);
            game.keyPressed('8');
            mapper.expect("/snake/down");
            mapper.sendGameOver(protocol);
            mapper.sendGameOver(protocol);
            mapper.sendGameOver(protocol);
            require(player.played ==
                        std::vector<AudioKey>{AudioKey::game_starts, AudioKey::game_over},
                    "Game over announcement must play exactly once");
            game.keyPressed('6');
            mapper.expectNothing();
            require(player.hangups == 0, "Hung up before the announcement finished");
            game.audioFinished(AudioKey::game_over);
            require(player.hangups == 1, "Not hung up after game over announcement");
            // The game already ended on the MadMapper side: no stop.
            mapper.expectNothing();
        }

        {
            // A stale game over from the previous player's stop is ignored.
            FakeClient previous;
            attach(game, previous);
            game.audioFinished(AudioKey::game_starts);
            game.keyPressed('6');
            mapper.expect("/snake/right");
            detach(previous);
            mapper.expect("/snake/stop");

            FakeClient next;
            attach(game, next);
            mapper.sendGameOver(protocol);
            game.audioFinished(AudioKey::game_starts);
            mapper.sendGameOver(protocol);
            require(next.played == std::vector<AudioKey>{AudioKey::game_starts},
                    "Stale game over reached the next player");
            require(next.hangups == 0, "Stale game over hung up the next player");
            game.keyPressed('2');
            mapper.expect("/snake/up");
            detach(next);
            mapper.expect("/snake/stop");
        }

        std::cout << "Game logic checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
