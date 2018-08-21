// This code is licensed under MIT license (see LICENSE.txt for details)
#include <iomanip>
#include <thread>
#include <time.h>

#include "../src/irc_client.h"
#include "../src/irc_commands.h"
#include "../src/irc_errors.h"
#include "../src/irc_replies.h"

using namespace std;
using namespace irclib;

std::string timestamp();

int main() {
    IrcRegistrationInfo registration_info;
    registration_info.nickname = "Twoflower";
    registration_info.username = "Twoflower";
    registration_info.realname = "Twoflower the Tourist";

    std::unique_ptr<IrcClient> client(new IrcClient());
    client->on(RPL_WELCOME, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] " << message.parameters[1] << "\r\n";
    });

    client->on(RPL_YOURHOST, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] " << message.parameters[1] << "\r\n";
    });

    client->on(RPL_CREATED, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] " << message.parameters[1] << "\r\n-\r\n";
    });

    client->on(RPL_HOSTHIDDEN, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] " << message.parameters[1]
                    << " is now your display host\r\n-\r\n";
    });

    client->on(RPL_LUSERCLIENT, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] " << message.parameters[1] << "\r\n";
    });

    client->on(RPL_LUSERME, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] " << message.parameters[1] << "\r\n";
    });

    client->on(RPL_LOCALUSERS, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] "
                    << "Current local  users:" << message.parameters[1]
                    << " Max: " << message.parameters[2] << "\r\n";
    });

    client->on(RPL_GLOBALUSERS, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] "
                    << "Current global users: " << message.parameters[1]
                    << " Max: " << message.parameters[2] << "\r\n-\r\n";
    });

    client->on(RPL_MOTDSTART, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] " << message.parameters[1] << "\r\n-\r\n";
    });

    client->on(RPL_MOTD, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] " << message.parameters[1] << "\r\n";
    });

    client->on(RPL_ENDOFMOTD, [](const IrcMessage message) {
        std::cout << "-\r\n";
        std::cout << "[" << timestamp() << "] " << message.parameters[1] << "\r\n-\r\n";
    });

    client->on(CMD_MODE, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] "
                    << "* " << message.parameters[0] << " sets mode " << message.parameters[1]
                    << "\r\n-\r\n";
    });

    client->on(CMD_JOIN, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] "
                    << "* " << message.source->getName() << " joined " << message.parameters[0]
                    << "\r\n";
    });

    client->on(CMD_PART, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] "
                    << "* " << message.source->getName() << " left " << message.parameters[0]
                    << "\r\n";
    });

    client->on(RPL_TOPIC, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] "
                    << "* " << message.parameters[1] << ": "
                    << "Topic is: '" << message.parameters[2] << "'"
                    << "\r\n";
    });

    client->on(CMD_TOPIC, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] "
                    << "* " << message.parameters[0] << ": " << message.source->getName()
                    << " changed the topic to '" << message.parameters[1] << "'"
                    << "\r\n";
    });
    client->on(RPL_TOPICWHOTIME, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] "
                    << "* " << message.parameters[1] << ": "
                    << "Set by " << message.parameters[2] << " on " << message.parameters[3]
                    << "\r\n";
    });

    client->on(CMD_PRIVMSG, [](const IrcMessage message) {
        std::cout << "[" << timestamp() << "] " << message.parameters[0] << ": "
                    << "<" << message.source->getName() << "> " << message.parameters[1]
                    << "\r\n";
    });

    client->on(PROTOCOL_ERROR, [](const IrcMessage message) {
        const int numeric_error = strtol(message.command.c_str(), nullptr, 10);
        if (numeric_error == ERR_UNKNOWNCOMMAND) {
            std::cout << "[" << timestamp() << "] Unknown Command.\r\n";
        } else {
            std::cout << "[" << timestamp() << "] ERROR (" << numeric_error << ")\r\n";
        }
    });

    client->on(NETWORK_ERROR, [](const char* error_message) {
        std::cout << "[" << timestamp() << "] Error: " << error_message << "\r\n";
    });

    std::cout << "[" << timestamp() << "] * Connecting to localhost:6667\r\n-\r\n";
    client->connect("localhost", 6667, registration_info);

    do {
        string line;
        getline(std::cin, line);

        client->sendRawMessage(line);

        if (_stricmp(line.c_str(), "QUIT") == 0) {
            break;
        }
    } while (true);

    return 0;
}

std::string timestamp() {
    time_t now = std::time(nullptr);

    struct tm timeinfo;
    localtime_s(&timeinfo, &now);

    stringstream ss;
    ss << std::put_time(&timeinfo, "%H:%M");

    return ss.str();
}
