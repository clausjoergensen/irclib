// This code is licensed under MIT license (see LICENSE.txt for details)
#pragma once

#include <string>
#include <vector>

namespace irclib {

class IrcClient;
class IrcMessageSource;

struct IrcMessage {
    irclib::IrcClient* client;
    std::string prefix;
    std::string command;
    std::vector<std::string> parameters;
    irclib::IrcMessageSource* source;
    std::string raw;
};

} // namespace irclib
