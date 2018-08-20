// This code is licensed under MIT license (see LICENSE.txt for details)
#include "../src/irc_client.h"
#include <crtdbg.h>
#include <thread>

using namespace std;
using namespace irclib;

int main() {
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

    IrcRegistrationInfo registration_info;
    registration_info.nickname = "Twoflower";
    registration_info.username = "Twoflower";
    registration_info.realname = "Twoflower the Tourist";

    std::unique_ptr<IrcClient> client(new IrcClient());
    std::thread t([&client, &registration_info] {
        client->on("message", [](const IrcMessage message) { std::cout << message.raw << "\r\n"; });
        client->connect("localhost", 6667, registration_info);
    });

    do {
        string line;
        getline(std::cin, line);

        client->sendRawMessage(line);

        if (line == "QUIT") {
            t.join();
            break;
        }
    } while (true);

    return 0;
}
