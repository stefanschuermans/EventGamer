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

enum class AudioKey { welcome, game_starts, game_over };

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
