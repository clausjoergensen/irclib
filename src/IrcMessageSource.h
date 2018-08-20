// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include <string>

namespace LibIrc {

class IrcMessageSource {
  public:
    virtual std::string getName() = 0;
};

} // namespace LibIrc
