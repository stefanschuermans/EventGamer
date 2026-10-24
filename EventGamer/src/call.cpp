#include "call.h"

#include <iostream>
#include <utility>

namespace eventgamer {

ActiveCall::ActiveCall(pj::Account &account, int call_id, const AudioConfig &audio)
    : pj::Call(account, call_id), audio_config_(audio) {}

void ActiveCall::playAudio(AudioKey audio) {
    if (disconnected_) {
        return;
    }
    auto next = std::make_unique<SoundPlayer>(audio);
    next->createPlayer(audio_config_.paths.at(audio).u8string(), PJMEDIA_FILE_NO_LOOP);

    const auto info = getInfo();
    for (const auto &media : info.media) {
        if (media.type == PJMEDIA_TYPE_AUDIO && media.status == PJSUA_CALL_MEDIA_ACTIVE) {
            auto sink = getAudioMedia(static_cast<int>(media.index));
            if (player_) {
                player_->stopTransmit(sink);
            }
            next->startTransmit(sink);
            break;
        }
    }
    // If media is not active yet, onCallMediaState will connect this player later.
    player_ = std::move(next);
}

void ActiveCall::processAudioEvents() {
    if (disconnected_ || !player_ || !player_->completed.load() || player_->notified) {
        return;
    }
    player_->notified = true;
    const auto audio = player_->audio;
    if (auto current_logic = logic) {
        // EOF only sets an atomic flag on the media thread. Here the logic may
        // safely replace/destroy the player or hang up on the main thread.
        current_logic->audioFinished(audio);
    }
}

void ActiveCall::hangUp() {
    setLogic(nullptr);
    if (disconnected_) {
        return;
    }
    pj::CallOpParam response;
    response.statusCode = PJSIP_SC_DECLINE;
    hangup(response);
}

void ActiveCall::setLogic(std::shared_ptr<Logic> new_logic) {
    if (auto previous_logic = logic) {
        previous_logic->clientDetaching();
    }
    logic = std::move(new_logic);
    if (auto current_logic = logic) {
        current_logic->clientAttached(*this);
    }
}

void ActiveCall::setQueuePosition(unsigned int position) {
    if (auto current_logic = logic) {
        current_logic->setQueuePosition(position);
    }
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
    if (!player_) {
        return;
    }
    try {
        const auto info = getInfo();
        for (const auto &media : info.media) {
            if (media.type == PJMEDIA_TYPE_AUDIO && media.status == PJSUA_CALL_MEDIA_ACTIVE) {
                auto audio = getAudioMedia(static_cast<int>(media.index));
                player_->startTransmit(audio);
                std::cout << "SIP call " << getId() << ": audio playback active" << std::endl;
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
        if (auto current_logic = logic; current_logic && !parameter.digit.empty()) {
            current_logic->keyPressed(parameter.digit.front());
        }
    }
}

} // namespace eventgamer
