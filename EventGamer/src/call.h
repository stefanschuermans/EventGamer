#pragma once

#include <pjsua2.hpp>

#include <filesystem>

namespace eventgamer {

class ActiveCall final : public pj::Call {
  public:
    ActiveCall(pj::Account &account, int call_id, const std::filesystem::path &queue_audio);

    void accept();
    bool finished() const { return disconnected_; }
    void onCallState(pj::OnCallStateParam &parameter) override;
    void onCallMediaState(pj::OnCallMediaStateParam &parameter) override;

  private:
    pj::AudioMediaPlayer player_;
    bool disconnected_ = false;
};

} // namespace eventgamer
