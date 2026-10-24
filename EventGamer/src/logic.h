#pragma once

#include "client.h"

namespace eventgamer {

class Logic {
  public:
    virtual ~Logic() = default;

    virtual void keyPressed(Client &client, char key) = 0;
    virtual void clientAttached(Client &client) = 0;
    virtual void clientDetaching(Client &client) = 0;
    virtual void audioFinished(Client &client, AudioKey audio) = 0;
    virtual void setQueuePosition(Client &client, unsigned int position) = 0;
};

} // namespace eventgamer
