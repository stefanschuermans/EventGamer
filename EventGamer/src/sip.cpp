#include "sip.h"
#include "shutdown.h"

#include <pjsua2.hpp>

#include <iostream>
#include <stdexcept>

namespace eventgamer {
namespace {

class SipAccount final : public pj::Account {
  public:
    ~SipAccount() override { shutdown(); }

    void onRegState(pj::OnRegStateParam &parameter) override {
        try {
            const auto info = getInfo();
            std::cout << "SIP registration: " << parameter.code
                      << (info.regIsActive ? " (registered)" : " (not registered)") << std::endl;
        } catch (const pj::Error &error) {
            std::cerr << "Cannot read SIP registration state: " << error.info() << std::endl;
        }
    }
};

// Declared before the account so the account is destroyed before the endpoint.
class SipEndpoint final {
  public:
    pj::Endpoint endpoint;

    SipEndpoint() { endpoint.libCreate(); }
    ~SipEndpoint() {
        try {
            endpoint.libDestroy();
        } catch (const pj::Error &error) {
            std::cerr << "SIP shutdown error: " << error.info() << std::endl;
        }
    }
    SipEndpoint(const SipEndpoint &) = delete;
    SipEndpoint &operator=(const SipEndpoint &) = delete;
};

} // namespace

void run_sip(const SipConfig &config) {
    // Keep URI construction unambiguous until separate domain/proxy settings exist.
    if (config.server.find_first_of(" \t\r\n@;/<>?#") != std::string::npos ||
        config.server.find("://") != std::string::npos || config.server.rfind("sip:", 0) == 0 ||
        config.server.rfind("sips:", 0) == 0) {
        throw std::runtime_error("sip.server must be a hostname or IP address with optional port, "
                                 "without a SIP URI prefix");
    }
    if (config.username.find_first_of(" \t\r\n@:/;<>?#") != std::string::npos) {
        throw std::runtime_error(
            "sip.username must be a SIP user name without a domain or URI prefix");
    }

    SipEndpoint runtime;
    auto &endpoint = runtime.endpoint;
    pj::EpConfig endpoint_config;
    endpoint_config.uaConfig.threadCnt = 0;
    endpoint_config.uaConfig.mainThreadOnly = true;
    // Registration does not need local sound hardware or SIP packet dumps.
    endpoint_config.logConfig.level = 3;
    endpoint_config.logConfig.consoleLevel = 3;
    endpoint_config.logConfig.msgLogging = false;
    endpoint.libInit(endpoint_config);

    pj::TransportConfig transport;
    transport.port = 0; // Let the OS choose an available local SIP port.
    endpoint.transportCreate(PJSIP_TRANSPORT_UDP, transport);
    endpoint.libStart();
    endpoint.audDevManager().setNullDev();

    SipAccount account;
    pj::AccountConfig account_config;
    account_config.idUri = "sip:" + config.username + "@" + config.server;
    account_config.regConfig.registrarUri = "sip:" + config.server;
    account_config.regConfig.registerOnAdd = true;
    account_config.regConfig.retryIntervalSec = 30;
    account_config.regConfig.firstRetryIntervalSec = 5;
    account_config.regConfig.unregWaitMsec = 2000;
    account_config.sipConfig.authCreds.emplace_back("digest", "*", config.username, 0,
                                                    config.password);
    if (shutdown_requested()) {
        return;
    }
    account.create(account_config);
    std::cout << "SIP started; waiting for registration and shutdown events." << std::endl;

    while (!shutdown_requested()) {
        const int result = endpoint.libHandleEvents(100);
        if (result < 0 && !shutdown_requested()) {
            throw std::runtime_error("SIP event processing failed: " + std::to_string(result));
        }
    }
    std::cout << "Shutting down SIP." << std::endl;
    // Account destruction initiates unregistration; libDestroy waits for it.
}

} // namespace eventgamer
