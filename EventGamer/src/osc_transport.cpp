#include "osc_transport.h"

#include <asio.hpp>

#include <array>
#include <iostream>
#include <stdexcept>

namespace eventgamer {

struct OSCTransport::Impl {
    asio::io_context context;
    asio::ip::udp::socket socket{context};
    asio::ip::udp::endpoint remote;
    std::array<char, 65536> buffer;

    explicit Impl(const OscConfig &config) {
        using asio::ip::udp;
        udp::resolver resolver(context);
        const auto local = resolver.resolve(config.bind_host, std::to_string(config.bind_port),
                                            udp::resolver::passive);
        const auto destinations =
            resolver.resolve(config.remote_host, std::to_string(config.remote_port));
        asio::error_code last_error;
        for (const auto &binding : local) {
            const bool ipv4 = binding.endpoint().address().is_v4();
            if ((config.address_family == AddressFamily::ipv4 && !ipv4) ||
                (config.address_family == AddressFamily::ipv6 && ipv4)) {
                continue;
            }
            for (const auto &destination : destinations) {
                if (destination.endpoint().protocol() != binding.endpoint().protocol()) {
                    continue;
                }
                socket.open(binding.endpoint().protocol(), last_error);
                if (last_error) {
                    continue;
                }
                // Auto chooses one common family; IPv6 sockets use IPv6 explicitly.
                if (!ipv4) {
                    socket.set_option(asio::ip::v6_only(true), last_error);
                }
                if (!last_error) {
                    socket.bind(binding.endpoint(), last_error);
                }
                if (!last_error) {
                    socket.non_blocking(true, last_error);
                }
                if (!last_error) {
                    remote = destination.endpoint();
                    std::cout << "OSC UDP listening on " << socket.local_endpoint() << "; remote "
                              << remote << std::endl;
                    return;
                }
                asio::error_code ignored;
                socket.close(ignored);
            }
        }
        throw std::runtime_error(
            "Cannot bind OSC UDP socket with a compatible remote address: " +
            (last_error ? last_error.message() : "no matching IPv4/IPv6 addresses"));
    }
};

OSCTransport::OSCTransport(const OscConfig &config) : impl_(std::make_unique<Impl>(config)) {}
OSCTransport::~OSCTransport() = default;

void OSCTransport::poll(const Receiver &receiver) {
    // Bound the work per iteration so a flood of UDP packets cannot starve SIP.
    for (unsigned count = 0; count < 64; ++count) {
        asio::ip::udp::endpoint sender;
        asio::error_code error;
        const auto size = impl_->socket.receive_from(asio::buffer(impl_->buffer), sender, 0, error);
        if (error == asio::error::would_block || error == asio::error::try_again) {
            return;
        }
        if (error) {
            std::cerr << "OSC receive error: " << error.message() << std::endl;
            return;
        }
        try {
            receiver(impl_->buffer.data(), size);
        } catch (const std::exception &exception) {
            std::cerr << "OSC packet rejected: " << exception.what() << std::endl;
        }
    }
}

void OSCTransport::send(const char *data, std::size_t size) {
    if (!data || size == 0 || size > 65507) {
        throw std::invalid_argument("Invalid OSC UDP packet size");
    }
    impl_->socket.send_to(asio::buffer(data, size), impl_->remote);
}

} // namespace eventgamer
