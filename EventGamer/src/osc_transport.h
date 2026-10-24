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
