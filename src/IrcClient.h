// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include "IrcRegistrationInfo.h"
#include "IrcMessage.h"
#include "IrcUser.h"
#include "IrcServer.h"

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

namespace LibIrc {

/**
 * Represents a client that communicates with a server using the IRC (Internet Relay Chat) protocol.
 */
class IrcClient {
public:
    /**
     * Initializes a new instance of the IrcClient class.
     */
    IrcClient();

    /**
     * Tears down the connection if open.
     */
    ~IrcClient();

    /**
     * Callback for when the IrcClients receives a new message.
     */
    std::function<void(IrcMessage)> onMessage;

    /**
     * Connects to the specified server.
     *
     * \param hostName The name of the remote host.
     * \param port The port number of the remote host.
     * \param registrationInfo The information used for registering the client.
     */
    void connect(std::string hostName, int port, LibIrc::IrcRegistrationInfo registrationInfo);

    /**
     * Sends the specified raw message to the server.
     *
     * \param message The text (single line) of the message to send the server. No CR/LF should be appended.
     */
    void sendRawMessage(std::string message);

private:
    void connected();
    void listen(std::string remainder = "");

    void parseMessage(std::string line);
    void processMessage(LibIrc::IrcMessage message);
    
    void writeMessage(std::string message);
    void writeMessage(std::string prefix, std::string command, std::vector<std::string> parameters);

    void sendMessagePassword(std::string password);
    void sendMessageNick(std::string nickName);
    void sendMessageUser(std::string userName, std::string realName, std::vector<char> userModes);
    void sendMessagePong(std::string ping);

    LibIrc::IrcMessageSource* getSourceFromPrefix(std::string prefix);
    LibIrc::IrcUser* getUserFromNickName(std::string nickName);
    LibIrc::IrcServer* getServerFromHostName(std::string hostName);

    std::string hostName;
    int port;
    LibIrc::IrcRegistrationInfo registrationInfo;

    WSADATA wsaData;
    SOCKET socket;

    std::vector<LibIrc::IrcUser*> users;
    std::vector<LibIrc::IrcServer*> servers;
};

}
