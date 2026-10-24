#pragma once

#include "client.h"

namespace eventgamer {

class Logic {
  public:
    virtual ~Logic() = default;

    virtual void keyPressed(char key) = 0;
    virtual void clientAttached(Client &client) = 0;
    virtual void clientDetaching() = 0;
    virtual void audioFinished(AudioKey audio) = 0;
    virtual void setQueuePosition(unsigned int position) = 0;
};

} // namespace eventgamer
