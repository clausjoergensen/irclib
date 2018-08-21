# irclib

A C++ port of jsIRC

### Example Usage

```cpp
using namespace irclib;

IrcRegistrationInfo registration_info;
registration_info.nickname = "Twoflower";
registration_info.username = "Twoflower";
registration_info.realname = "Twoflower the Tourist";

std::unique_ptr<IrcClient> client(new IrcClient());

client->on(RPL_TOPIC, [](const IrcMessage message) {
    std::cout << "Topic is: '" << message.parameters[2] << "'\r\n";
});

client->connect("localhost", 6667, registration_info);

client->sendRawMessage("JOIN ##c++");
client->sendRawMessage("PRIVMSG ##c++ hi everyone");
client->sendRawMessage("QUIT");
````

Also see [test.cpp](test/test.cpp) for a full usage example.

### Example Output

```
[14:28] * Connecting to localhost:6667
-
[14:28] Welcome to the C++ IRC Network Twoflower!Twoflower@localhost
[14:28] Your host is irc.local.host, running version UnrealIRCd-4.0.18
[14:28] This server was created Sat Jun 23 16:12:43 2018
-
[14:28] Clk-7E3F3D83 is now your display host
-
[14:28] There are 1 users and 1 invisible on 1 servers
[14:28] I have 2 clients and 0 servers
[14:28] Current local  users:2 Max: 5
[14:28] Current global users: 2 Max: 2
-
[14:28] - irc.local.host Message of the Day -
-
[14:28] - 20/8/2018 2:31
[14:28] -
[14:28] -            Welcome to the C++ IRC Network!     
[14:28] -
-
[14:28] End of /MOTD command.
-
[14:28] * Twoflower sets mode +iwx
-
JOIN ##c++
-
[14:28] * Twoflower joined ##c++
[14:28] * ##c++: Topic is: '##c++: is a topical channel for discussing standard C++ specifications and code.'
[14:28] * ##c++: Set by Windcape on 1534854447
-
PRIVMSG ##c++ hi everyone
-
[14:28] ##c++: <Windcape> hi Twoflower!
```
