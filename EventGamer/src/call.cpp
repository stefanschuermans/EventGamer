#include "call.h"

#include <iostream>

namespace eventgamer {

ActiveCall::ActiveCall(pj::Account &account, int call_id, const std::filesystem::path &queue_audio)
    : pj::Call(account, call_id) {
    // Default player options loop the file. Each call has its own playback position.
    player_.createPlayer(queue_audio.u8string());
}

void ActiveCall::accept() {
    pj::CallOpParam response;
    response.statusCode = PJSIP_SC_OK;
    response.opt.audioCount = 1;
    response.opt.videoCount = 0;
    answer(response);
    std::cout << "Accepted SIP call " << getId() << std::endl;
}

void ActiveCall::onCallState(pj::OnCallStateParam &) {
    try {
        const auto info = getInfo();
        std::cout << "SIP call " << getId() << ": " << info.stateText << std::endl;
        if (info.state == PJSIP_INV_STATE_DISCONNECTED) {
            // The account removes this object after the callback has returned.
            disconnected_ = true;
        }
    } catch (const pj::Error &error) {
        std::cerr << "Cannot read call state: " << error.info() << std::endl;
    }
}

void ActiveCall::onCallMediaState(pj::OnCallMediaStateParam &) {
    try {
        const auto info = getInfo();
        for (const auto &media : info.media) {
            if (media.type == PJMEDIA_TYPE_AUDIO && media.status == PJSUA_CALL_MEDIA_ACTIVE) {
                auto audio = getAudioMedia(static_cast<int>(media.index));
                player_.startTransmit(audio);
                std::cout << "SIP call " << getId() << ": playing in_queue in a loop" << std::endl;
                break;
            }
        }
    } catch (const pj::Error &error) {
        std::cerr << "Cannot play queue audio: " << error.info() << std::endl;
        // A caller without working queue audio cannot participate yet.
        try {
            pj::CallOpParam response;
            response.statusCode = PJSIP_SC_INTERNAL_SERVER_ERROR;
            hangup(response);
        } catch (const pj::Error &hangup_error) {
            std::cerr << "Cannot end call: " << hangup_error.info() << std::endl;
        }
    }
}

void ActiveCall::onDtmfEvent(pj::OnDtmfEventParam &parameter) {
    if (!(parameter.flags & PJMEDIA_STREAM_DTMF_IS_UPDATE)) {
        std::cout << "SIP call " << getId() << ": key " << parameter.digit << " pressed"
                  << std::endl;
    }
}

} // namespace eventgamer
