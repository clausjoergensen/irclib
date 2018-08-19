// Copyright (c) 2018 Claus Jørgensen
#pragma once
#include <string>

namespace LibIrc
{
    struct IrcRegistrationInfo
    {
        std::string nickName;
        std::string userName;
        std::string realName;
        char userModes[10];
    };
}
