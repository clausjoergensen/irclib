// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include "stdafx.h"

#include "EventEmitter.h"
#include "IrcMessage.h"
#include "IrcRegistrationInfo.h"
#include "IrcServer.h"
#include "IrcUser.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace LibIrc {

/**
 * Represents a client that communicates with a server using the IRC (Internet
 * Relay Chat) protocol.
 */
class IrcClient : public EventEmitter {
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
     * \param message The text (single line) of the message to send the server.
     * No CR/LF should be appended.
     */
    void sendRawMessage(std::string message);

    /**
     * Gets the local user (or a nullptr before registering).
     */
    IrcLocalUser* getLocalUser() {
        return this->localUser;
    }

  private:
    void connected();
    void listen(std::string remainder = "");

    void parseMessage(std::string line);

    void processMessage(LibIrc::IrcMessage message);
    void processMessagePing(LibIrc::IrcMessage message);

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
    LibIrc::IrcLocalUser* localUser;

    WSADATA wsaData;
    SOCKET socket;

    std::vector<LibIrc::IrcUser*> users;
    std::vector<LibIrc::IrcServer*> servers;
};

} // namespace LibIrc
