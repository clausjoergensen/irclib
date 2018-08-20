// This code is licensed under MIT license (see LICENSE.txt for details)
#pragma once

#include "irc_message_source.h"

namespace irclib {

class IrcUser : public IrcMessageSource {
  public:
    IrcUser(std::string nickname) 
        : nickname(nickname) {}

    ~IrcUser() {}

    std::string nickname;
    std::string username;
    std::string hostname;

    std::string getName() {
        return this->nickname;
    }

    bool isLocalUser() {
        return false;
    }
};

class IrcLocalUser : public IrcUser {
  public:
    IrcLocalUser(std::string nickname) 
        : IrcUser(nickname) {}

    ~IrcLocalUser() {}

  public:
    bool isLocalUser() {
        return true;
    }
};

} // namespace irclib
