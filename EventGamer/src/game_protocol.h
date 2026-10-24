#pragma once

#include "osc_transport.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace eventgamer {

using OSCArgument = std::variant<std::int32_t, float, std::string, bool, std::int64_t, double>;
struct OSCMessage {
    std::string address;
    std::vector<OSCArgument> arguments;
};

class GameProtocol final {
  public:
    using Handler = std::function<void(const OSCMessage &)>;
    explicit GameProtocol(OSCTransport &transport);
    void poll();
    void sendMessage(const OSCMessage &message);
    // Exact OSC address matching; replace a handler by registering it again.
    void setHandler(const std::string &address, Handler handler);
    // Receives messages without an address-specific handler. Empty clears it.
    void setHandler(Handler handler);

  private:
    void receivePacket(const char *data, std::size_t size, unsigned depth = 0);
    OSCTransport &transport_;
    std::map<std::string, Handler> handlers_;
    Handler default_handler_;
};

} // namespace eventgamer
