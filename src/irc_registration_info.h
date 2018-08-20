// This code is licensed under MIT license (see LICENSE.txt for details)
#pragma once

#include <string>
#include <vector>

namespace irclib {

struct IrcRegistrationInfo {
    std::string nickname;
    std::string username;
    std::string realname;
    std::string password;
    std::vector<char> user_modes;
};

} // namespace irclib
