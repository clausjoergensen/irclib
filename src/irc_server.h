// This code is licensed under MIT license (see LICENSE.txt for details)
#pragma once

#include "irc_message_source.h"

namespace irclib {

class IrcServer : public IrcMessageSource {
  public:
    IrcServer(std::string hostname) 
        : hostname(hostname) {}
    
    ~IrcServer() {}

    std::string hostname;

    std::string getName() {
        return this->hostname;
    }
};

} // namespace irclib
