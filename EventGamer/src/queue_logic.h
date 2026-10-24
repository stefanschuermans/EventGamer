#pragma once

#include "logic.h"

namespace eventgamer {

class QueueLogic final : public Logic {
  public:
    explicit QueueLogic();
    QueueLogic(const QueueLogic &) = delete;
    QueueLogic &operator=(const QueueLogic &) = delete;
    void keyPressed(char key) override;
    void clientAttached(Client &client) override;
    void clientDetaching() override;
    void audioFinished(AudioKey audio) override;
    void setQueuePosition(unsigned int position) override;

  private:
    Client *client_;
};

} // namespace eventgamer
