/*
 * EventGamer - telephone controls for Snake in MadMapper.
 * Copyright (C) 2026 Stefan Schuermans
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
    void playAudioLoop(AudioKey audio) override;
    void hangUp() override;
    void processAudioEvents();
    bool finished() const { return disconnected_; }
    void onCallState(pj::OnCallStateParam &parameter) override;
    void onCallMediaState(pj::OnCallMediaStateParam &parameter) override;
    void onDtmfEvent(pj::OnDtmfEventParam &parameter) override;

  private:
    class SoundPlayer final : public pj::AudioMediaPlayer {
      public:
        SoundPlayer(AudioKey key, bool repeat) : audio(key), loop(repeat) {}
        // A looping player reaches EOF on every repetition and never completes.
        void onEof2() override {
            if (!loop)
                completed.store(true);
        }

        const AudioKey audio;
        const bool loop;
        std::atomic<bool> completed{false};
        bool notified = false; // Accessed only by the main thread.
    };

    void startPlayer(AudioKey audio, bool loop);

    AudioConfig audio_config_;
    std::unique_ptr<SoundPlayer> player_;
    bool disconnected_ = false;
};

} // namespace eventgamer
