// Copyright (c) 2018 Claus Jørgensen
#include "IrcClient.h"
#include <thread>

using namespace std;
using namespace LibIrc;

int main()
{
    IrcRegistrationInfo registrationInfo;
    registrationInfo.nickName = "Twoflower";
    registrationInfo.userName = "Twoflower";
    registrationInfo.realName = "Twoflower the Tourist";

    auto client = new IrcClient();
    std::thread t([&client,&registrationInfo] {
        client->connect("localhost", 6667, registrationInfo);
    });

    do 
     {
        string line;
        getline(std::cin, line);

        client->sendRawMessage(line + "\r\n");

        if (line == "QUIT") {
            t.join();
            break;
        }   		
     } while (true);


    return 0;
}
