// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include <string>
#include <vector>

namespace LibIrc
{
    struct IrcRegistrationInfo
    {
        std::string nickName;
        std::string userName;
        std::string realName;
        std::string password;
        std::vector<char> userModes;
    };
}
