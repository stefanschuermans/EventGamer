#pragma once

#include "logic.h"

namespace eventgamer {

class QueueLogic final : public Logic {
  public:
    void keyPressed(Client &client, char key) override;
    void clientAttached(Client &client) override;
    void clientDetaching(Client &client) override;
    void audioFinished(Client &client, AudioKey audio) override;
    void setQueuePosition(Client &client, unsigned int position) override;
};

} // namespace eventgamer
