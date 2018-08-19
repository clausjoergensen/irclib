// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include "IrcMessageSource.h"

namespace LibIrc
{
    class IrcUser : public IrcMessageSource
    {
    public:
        IrcUser();
        ~IrcUser();
    public:
        std::string nickName;
        std::string userName;
        std::string hostName;
    public:
        std::string getName() 
        {
            return this->nickName;
        }
        bool isLocalUser() 
        {
            return false;
        }
    };

    class IrcLocalUser : public IrcUser 
    {
    public:
        IrcLocalUser();
        ~IrcLocalUser();
    public:
        bool isLocalUser() 
        {
            return true;
        }
    };
}
