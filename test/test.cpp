// Copyright (c) 2018 Claus Jørgensen

#include "../src/IrcClient.h"
#include <thread>
#include <crtdbg.h>

using namespace std;
using namespace LibIrc;

int main() {
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

    IrcRegistrationInfo registrationInfo;
    registrationInfo.nickName = "Twoflower";
    registrationInfo.userName = "Twoflower";
    registrationInfo.realName = "Twoflower the Tourist";

    std::unique_ptr<IrcClient> client(new IrcClient());
    std::thread t([&client, &registrationInfo] {
        client->on("message", [](const IrcMessage message) {
            std::cout << message.raw << "\r\n";
        });
        client->connect("localhost", 6667, registrationInfo);
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
