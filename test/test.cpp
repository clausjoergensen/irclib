// This code is licensed under MIT license (see LICENSE.txt for details)
#include <crtdbg.h>
#include <iomanip>
#include <thread>
#include <time.h>

#include "../src/irc_client.h"
#include "../src/irc_commands.h"
#include "../src/irc_errors.h"
#include "../src/irc_replies.h"

using namespace std;
using namespace irclib;

int getConsolePositionY() {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO console_screen_buffer_info;
    GetConsoleScreenBufferInfo(console_handle, &console_screen_buffer_info);
    return console_screen_buffer_info.dwCursorPosition.Y;
}

const LPWSTR WSAFormatError(const int error_code) {
    LPWSTR error_string;

    int size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM, // windows internal message table
                             0,          // 0, since source is internal message table
                             error_code, // error code returned by WSAGetLastError()
                             0,          // 0, auto-determine which language to use.
                             (LPWSTR)&error_string,
                             0,  // buffer minimum size.
                             0); // 0, since getting message from system tables

    if (size == 0) {
        return NULL;
    }

    return error_string;
}

int main() {
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

    IrcRegistrationInfo registration_info;
    registration_info.nickname = "Twoflower";
    registration_info.username = "Twoflower";
    registration_info.realname = "Twoflower the Tourist";

    std::unique_ptr<IrcClient> client(new IrcClient());
    std::thread t([&client, &registration_info] {
        client->on("message", [](const IrcMessage message) {
            auto now = std::time(nullptr);
            struct tm timeinfo;
            localtime_s(&timeinfo, &now);
            auto timestamp = std::put_time(&timeinfo, "%H:%M");

            if (message.command == RPL_WELCOME) {
                std::cout << "[" << timestamp << "] " << message.parameters[1] << "\r\n";
            } else if (message.command == RPL_YOURHOST) {
                std::cout << "[" << timestamp << "] " << message.parameters[1] << "\r\n";
            } else if (message.command == RPL_CREATED) {
                std::cout << "[" << timestamp << "] " << message.parameters[1] << "\r\n-\r\n";
            } else if (message.command == RPL_MYINFO) {
                // Ignored
            } else if (message.command == RPL_ISUPPORT) {
                // Ignored
            } else if (message.command == RPL_HOSTHIDDEN) {
                std::cout << "[" << timestamp << "] " << message.parameters[1]
                          << " is now your display host\r\n-\r\n";
            } else if (message.command == RPL_LUSERCLIENT) {
                std::cout << "[" << timestamp << "] " << message.parameters[1] << "\r\n";
            } else if (message.command == RPL_LUSERME) {
                std::cout << "[" << timestamp << "] " << message.parameters[1] << "\r\n";
            } else if (message.command == RPL_LOCALUSERS) {
                std::cout << "[" << timestamp << "] "
                          << "Current local  users:" << message.parameters[1]
                          << " Max: " << message.parameters[2] << "\r\n";
            } else if (message.command == RPL_GLOBALUSERS) {
                std::cout << "[" << timestamp << "] "
                          << "Current global users: " << message.parameters[1]
                          << " Max: " << message.parameters[2] << "\r\n-\r\n";
            } else if (message.command == RPL_MOTDSTART) {
                std::cout << "[" << timestamp << "] " << message.parameters[1] << "\r\n-\r\n";
            } else if (message.command == RPL_MOTD) {
                std::cout << "[" << timestamp << "] " << message.parameters[1] << "\r\n";
            } else if (message.command == RPL_ENDOFMOTD) {
                std::cout << "-\r\n";
                std::cout << "[" << timestamp << "] " << message.parameters[1] << "\r\n-\r\n";
            } else if (message.command == CMD_MODE) {
                std::cout << "[" << timestamp << "] "
                          << "* " << message.parameters[0] << " sets mode " << message.parameters[1]
                          << "\r\n-\r\n";
            } else if (message.command == CMD_JOIN) {
                std::cout << "[" << timestamp << "] "
                          << "* " << message.source->getName() << " joined "
                          << message.parameters[0] << "\r\n";
            } else if (message.command == CMD_PART) {
                std::cout << "[" << timestamp << "] "
                          << "* " << message.source->getName() << " left " << message.parameters[0]
                          << "\r\n";
            } else if (message.command == RPL_TOPIC) {
                std::cout << "[" << timestamp << "] "
                          << "* " << message.parameters[1] << ": "
                          << "Topic is: '" << message.parameters[0] << "'"
                          << "\r\n";
            } else if (message.command == CMD_TOPIC) {
                std::cout << "[" << timestamp << "] "
                          << "* " << message.parameters[0] << ": " << message.source->getName()
                          << " changed the topic to '" << message.parameters[1] << "'"
                          << "\r\n";
            } else if (message.command == RPL_TOPICWHOTIME) {
                std::cout << "[" << timestamp << "] "
                          << "* " << message.parameters[1] << ": "
                          << "Set by " << message.parameters[2] << " on " << message.parameters[3]
                          << "\r\n";
            } else if (message.command == RPL_NAMREPLY) {
                // Ignored
            } else if (message.command == RPL_ENDOFNAMES) {
                // Ignored
            } else if (message.command == CMD_PRIVMSG) {
                std::cout << "[" << timestamp << "] " << message.parameters[0] << ": "
                          << "<" << message.source->getName() << "> " << message.parameters[1]
                          << "\r\n";
            } else {
                auto numericError = atoi(message.command.c_str());
                if (numericError >= 400 && numericError <= 599) {
                    if (numericError == ERR_UNKNOWNCOMMAND) {
                        std::cout << "[" << timestamp << "] Unknown command\r\n";
                    } else {
                        std::cout << "[" << timestamp << "] ERROR (" << numericError << ")\r\n";
                    }
                } else {
                    std::cout << "[" << timestamp << "] " << message.raw << "\r\n";
                }
            }
        });

        auto now = std::time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        auto timestamp = std::put_time(&timeinfo, "%H:%M");
        std::cout << "-\r\n";
        std::cout << "[" << timestamp << "] * Connecting to localhost:6667\r\n-\r\n";

        client->connect("localhost", 6667, registration_info);
    });

    do {
        string line;
        getline(std::cin, line);

        client->sendRawMessage(line);

        if (_stricmp(line.c_str(), "QUIT") == 0) {
            t.join();
            break;
        }
    } while (true);

    return 0;
}
