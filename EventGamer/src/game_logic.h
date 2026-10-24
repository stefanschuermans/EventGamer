#pragma once

#include "logic.h"

#include <iosfwd>

namespace eventgamer {

class GameProtocol;
struct OSCMessage;

class GameLogic final : public Logic {
  public:
    explicit GameLogic(GameProtocol &protocol);
    ~GameLogic() override;
    GameLogic(const GameLogic &) = delete;
    GameLogic &operator=(const GameLogic &) = delete;
    void messageReceived(const OSCMessage &message);
    void keyPressed(char key) override;
    void clientAttached(Client &client) override;
    void clientDetaching() override;
    void audioFinished(AudioKey audio) override;
    void setQueuePosition(unsigned int position) override;

  private:
    void dumpMessage(const OSCMessage &message, std::ostream &stream);
    void onGameOver();
    void sendMessage(const OSCMessage &message);
    void sendUp();
    void sendDown();
    void sendLeft();
    void sendRight();
    void sendRestart();
    void sendStop();

    GameProtocol &protocol_;
    Client *client_;

    bool game_over_;
};

} // namespace eventgamer
