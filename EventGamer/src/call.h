#pragma once

#include "client.h"
#include "logic.h"

#include <pjsua2.hpp>

#include <atomic>
#include <memory>

namespace eventgamer {

class ActiveCall final : public pj::Call, public Client {
  public:
    ActiveCall(pj::Account &account, int call_id, const AudioConfig &audio);

    std::shared_ptr<Logic> logic;

    void accept();
    void setLogic(std::shared_ptr<Logic> new_logic);
    void setQueuePosition(unsigned int position);
    void playAudio(AudioKey audio) override;
    void hangUp() override;
    void processAudioEvents();
    bool finished() const { return disconnected_; }
    void onCallState(pj::OnCallStateParam &parameter) override;
    void onCallMediaState(pj::OnCallMediaStateParam &parameter) override;
    void onDtmfEvent(pj::OnDtmfEventParam &parameter) override;

  private:
    class SoundPlayer final : public pj::AudioMediaPlayer {
      public:
        explicit SoundPlayer(AudioKey key) : audio(key) {}
        void onEof2() override { completed.store(true); }

        const AudioKey audio;
        std::atomic<bool> completed{false};
        bool notified = false; // Accessed only by the main thread.
    };

    AudioConfig audio_config_;
    std::unique_ptr<SoundPlayer> player_;
    bool disconnected_ = false;
};

} // namespace eventgamer
