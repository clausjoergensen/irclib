// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include <string>
#include <vector>

namespace LibIrc
{
    class IrcClient;
    class IrcMessageSource;

    struct IrcMessage
    {
        IrcClient *client;
        std::string prefix;
        std::string command;
        std::vector<std::string> parameters;
        IrcMessageSource *source;
        std::string raw;
    };
}
