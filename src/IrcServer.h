// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include "IrcMessageSource.h"

namespace LibIrc {

class IrcServer : public IrcMessageSource {
  public:
    IrcServer();
    ~IrcServer();

    std::string hostname;

    std::string getName() {
        return this->hostname;
    }
};

} // namespace LibIrc
