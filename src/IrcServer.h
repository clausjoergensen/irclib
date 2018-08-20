// This code is licensed under MIT license (see LICENSE.txt for details)
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
