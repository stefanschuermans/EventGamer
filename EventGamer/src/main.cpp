#include "config.h"
#include "shutdown.h"
#include "sip.h"

#include <pjsua2.hpp>
#include <toml++/toml.hpp>

#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char *argv[]) {
    if (argc == 2 && std::string_view{argv[1]} == "--help") {
        std::cout << "Usage: EventGamer <config.toml> [--sip-trace]\n";
        return 0;
    }
    if ((argc != 2 && argc != 3) || (argc == 3 && std::string_view{argv[2]} != "--sip-trace")) {
        std::cerr << "Usage: EventGamer <config.toml> [--sip-trace]\n";
        return 1;
    }
    try {
        eventgamer::install_shutdown_handlers();
        const auto config = eventgamer::load_config(std::filesystem::u8path(argv[1]));
        std::cout << "Configuration loaded successfully.\n";
        eventgamer::run_sip(config.sip, config.audio, argc == 3);
        return 0;
    } catch (const toml::parse_error &error) {
        std::cerr << "Configuration parse error: " << error << '\n';
    } catch (const pj::Error &error) {
        std::cerr << "SIP error: " << error.info() << '\n';
    } catch (const std::exception &error) {
        std::cerr << "EventGamer error: " << error.what() << '\n';
    }
    return 1;
}
