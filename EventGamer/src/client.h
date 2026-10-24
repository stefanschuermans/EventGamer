#pragma once

#include "config.h"

namespace eventgamer {

class Client {
  public:
    virtual ~Client() = default;

    virtual void playAudio(AudioKey audio) = 0;
    virtual void hangUp() = 0;
};

} // namespace eventgamer
