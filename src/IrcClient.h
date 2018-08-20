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
    void connect(const std::string hostNameconst, const int port,
                 const LibIrc::IrcRegistrationInfo registrationInfo);

    /**
     * Sends the specified raw message to the server.
     *
     * \param message The text (single line) of the message to send the server.
     * No CR/LF should be appended.
     */
    void sendRawMessage(const std::string message);

    /**
     * Gets the local user (or a nullptr before registering).
     */
    const IrcLocalUser* getLocalUser() {
        return this->localUser;
    }

  private:
    void connected();
    void listen(const std::string remainder = "");

    void parseMessage(const std::string line);

    void processMessage(const LibIrc::IrcMessage message);
    void processMessagePing(const LibIrc::IrcMessage message);

    void writeMessage(const std::string message);
    void writeMessage(const std::string prefix, const std::string command,
                      const std::vector<std::string> parameters);

    void sendMessagePassword(const std::string password);
    void sendMessageNick(const std::string nickName);
    void sendMessageUser(const std::string userName, const std::string realName,
                         const std::vector<char> userModes);
    void sendMessagePong(const std::string ping);

    LibIrc::IrcMessageSource* getSourceFromPrefix(const std::string prefix);
    LibIrc::IrcUser* getUserFromNickName(const std::string nickName);
    LibIrc::IrcServer* getServerFromHostName(const std::string hostName);

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
