#include "sip.h"
#include "call.h"
#include "game_logic.h"
#include "queue_logic.h"
#include "shutdown.h"

#include <pjsua2.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace eventgamer {
namespace {

class SipAccount final : public pj::Account {
  public:
    SipAccount(const AudioConfig &audio, std::shared_ptr<GameLogic> game_logic)
        : audio_config_(audio), game_logic_(std::move(game_logic)) {}

    ~SipAccount() override {
        // Call destructors detach callbacks and hang up any remaining calls while
        // their account and the endpoint are still alive.
        calls_.clear();
        shutdown();
    }

    void remove_finished_calls() {
        const auto previous_size = calls_.size();
        calls_.erase(std::remove_if(calls_.begin(), calls_.end(),
                                    [](const auto &call) { return call->finished(); }),
                     calls_.end());
        if (calls_.size() != previous_size) {
            update_queue_positions();
        }
    }

    void onIncomingCall(pj::OnIncomingCallParam &parameter) override {
        std::cout << "Incoming SIP call " << parameter.callId << std::endl;
        try {
            auto call = std::make_unique<ActiveCall>(*this, parameter.callId, audio_config_);
            call->setLogic(std::make_shared<QueueLogic>());
            auto &active = *call;
            // Incoming callbacks and answers run serially on main. Append before
            // answer(), which may itself invoke call callbacks synchronously.
            calls_.push_back(std::move(call));
            update_queue_positions();
            active.accept();
        } catch (const pj::Error &error) {
            std::cerr << "Cannot accept incoming call: " << error.info() << std::endl;
            end_failed_call(parameter.callId);
        } catch (const std::exception &error) {
            std::cerr << "Cannot accept incoming call: " << error.what() << std::endl;
            end_failed_call(parameter.callId);
        }
    }

    void process_audio_events() {
        for (const auto &call : calls_) {
            call->processAudioEvents();
        }
    }

    void onRegState(pj::OnRegStateParam &parameter) override {
        try {
            const auto info = getInfo();
            std::cout << "SIP registration: " << parameter.code
                      << (info.regIsActive ? " (registered)" : " (not registered)") << std::endl;
        } catch (const pj::Error &error) {
            std::cerr << "Cannot read SIP registration state: " << error.info() << std::endl;
        }
    }

  private:
    void update_queue_positions() {
        if (!calls_.empty() && calls_.front()->logic != game_logic_) {
            calls_.front()->setLogic(game_logic_);
        }
        for (std::size_t index = 0; index < calls_.size(); ++index) {
            calls_[index]->setQueuePosition(static_cast<unsigned int>(index));
        }
    }

    void end_failed_call(int call_id) {
        for (const auto &call : calls_) {
            if (call->getId() == call_id && !call->finished()) {
                try {
                    pj::CallOpParam response;
                    response.statusCode = PJSIP_SC_INTERNAL_SERVER_ERROR;
                    call->hangup(response);
                } catch (const pj::Error &error) {
                    std::cerr << "Cannot end failed call: " << error.info() << std::endl;
                }
                break;
            }
        }
    }

    AudioConfig audio_config_;
    std::shared_ptr<GameLogic> game_logic_;
    // Oldest accepted call first; erase preserves the order of remaining calls.
    std::vector<std::unique_ptr<ActiveCall>> calls_;
};

// Declared before the account so the account is destroyed before the endpoint.
class DiagnosticEndpoint final : public pj::Endpoint {
  public:
    void onRejectedIncomingCall(pj::OnRejectedIncomingCallParam &parameter) override {
        std::cerr << "Incoming SIP call rejected by PJSIP: " << parameter.statusCode << " "
                  << parameter.reason << std::endl;
    }
};

class SipEndpoint final {
  public:
    DiagnosticEndpoint endpoint;

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

void run_sip(const SipConfig &config, const AudioConfig &audio, bool sip_trace) {
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
    endpoint_config.logConfig.level = sip_trace ? 5 : 3;
    endpoint_config.logConfig.consoleLevel = sip_trace ? 4 : 3;
    endpoint_config.logConfig.msgLogging = sip_trace;
    endpoint.libInit(endpoint_config);

    pj::TransportConfig transport;
    transport.port = 0; // Let the OS choose an available local SIP port.
    const auto transport_id = endpoint.transportCreate(PJSIP_TRANSPORT_UDP, transport);
    const auto transport_info = endpoint.transportGetInfo(transport_id);
    std::cout << "SIP UDP listening on " << transport_info.localAddress << "; advertised address "
              << transport_info.localName << std::endl;
    endpoint.libStart();
    endpoint.audDevManager().setNullDev();

    auto game_logic = std::make_shared<GameLogic>();
    SipAccount account(audio, game_logic);
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
        account.remove_finished_calls();
        account.process_audio_events();
        if (result < 0 && !shutdown_requested()) {
            throw std::runtime_error("SIP event processing failed: " + std::to_string(result));
        }
    }
    std::cout << "Shutting down SIP." << std::endl;
    // Account destruction initiates unregistration; libDestroy waits for it.
}

} // namespace eventgamer
