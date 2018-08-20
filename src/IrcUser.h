// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include "IrcMessageSource.h"

namespace LibIrc {

class IrcUser : public IrcMessageSource {
  public:
    IrcUser();
    ~IrcUser();

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
    IrcLocalUser();
    ~IrcLocalUser();

  public:
    bool isLocalUser() {
        return true;
    }
};

} // namespace LibIrc
