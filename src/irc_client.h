// This code is licensed under MIT license (see LICENSE.txt for details)
#pragma once

#include "pch.h"

#include "events.h"

#include "irc_message.h"
#include "irc_registration_info.h"
#include "irc_server.h"
#include "irc_user.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace irclib {

// Represents a client that communicates with a server using the IRC (Internet
// Relay Chat) protocol.
class IrcClient : public events::EventEmitter {
  public:
    // Initializes a new instance of the IrcClient class.
    IrcClient();

    // Tears down the connection if open.
    ~IrcClient();

    // Connects to the specified server.
    //
    // \param hostname The name of the remote host.
    // \param port The port number of the remote host.
    // \param registration_info The information used for registering the client.
    void connect(const std::string hostname, const int port,
                 const irclib::IrcRegistrationInfo registration_info);

    //Sends the specified raw message to the server.
    // \param message The text (single line) of the message to send the server.
    // No CR/LF should be appended.
    void sendRawMessage(const std::string message);

    // Gets the local user (or a nullptr before registering).
    const irclib::IrcLocalUser* getLocalUser() {
        return this->local_user;
    }

  private:
    void connected();
    void listen(const std::string remainder = "");

    void parseMessage(const std::string line);

    void processMessage(const irclib::IrcMessage message);
    void processMessagePing(const irclib::IrcMessage message);

    void writeMessage(const std::string message);
    void writeMessage(const std::string prefix, const std::string command,
                      const std::vector<std::string> parameters);

    void sendMessagePassword(const std::string password);
    void sendMessageNick(const std::string nickname);
    void sendMessageUser(const std::string username, const std::string realname,
                         const std::vector<char> user_modes);
    void sendMessagePong(const std::string ping);

    irclib::IrcMessageSource* getSourceFromPrefix(const std::string prefix);
    irclib::IrcUser* getUserFromNickName(const std::string nickname);
    irclib::IrcServer* getServerFromHostName(const std::string hostname);

    std::string hostname;
    int port;
    irclib::IrcRegistrationInfo registration_info;
    irclib::IrcLocalUser* local_user;

    ::WSADATA wsadata;
    ::SOCKET socket;

    std::vector<irclib::IrcUser*> users;
    std::vector<irclib::IrcServer*> servers;
};

} // namespace irclib