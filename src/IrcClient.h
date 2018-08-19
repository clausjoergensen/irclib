// Copyright (c) 2018 Claus Jørgensen
#pragma once
#include "IrcRegistrationInfo.h"
#include "IrcMessage.h"
#include "IrcUser.h"
#include "IrcServer.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

namespace LibIrc
{
    class IrcClient
    {
    public:
        IrcClient();
        ~IrcClient();
    public:
        std::function<void(IrcMessage)> onMessage;
        void connect(std::string hostName, int port, LibIrc::IrcRegistrationInfo registrationInfo);
        void sendRawMessage(std::string message);
    private:
        void parseMessage(std::string line);
        void processMessage(LibIrc::IrcMessage message);
        void writeMessage(std::string prefix, std::string command, std::vector<std::string> parameters);
    private:
        void sendMessageNick(std::string nickName);
        void sendMessageUser(std::string userName, std::string realName);
        void sendMessagePong(std::string ping);
    private:
        LibIrc::IrcMessageSource* getSourceFromPrefix(std::string prefix);
        LibIrc::IrcUser* getUserFromNickName(std::string nickName);
        LibIrc::IrcServer* getServerFromHostName(std::string hostName);
    private:
        std::string hostName;
        int port;
        LibIrc::IrcRegistrationInfo registrationInfo;
    private:
        WSADATA wsaData;
        SOCKET socket;
    private:
        std::vector<LibIrc::IrcUser*> users;
        std::vector<LibIrc::IrcServer*> servers;
    };
}
