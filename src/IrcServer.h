// Copyright (c) 2018 Claus Jørgensen
#pragma once
#include "IrcMessageSource.h"

namespace LibIrc
{
    class IrcServer: public IrcMessageSource
    {
    public:
        IrcServer();
        ~IrcServer();
    public:
        std::string hostName;
    public:
        std::string getName()
        {
            return this->hostName;
        }
    };
}
