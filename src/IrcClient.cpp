// Copyright (c) 2018 Claus Jørgensen

#include "stdafx.h"

#include "IrcClient.h"
#include "IrcCommand.h"
#include "IrcError.h"
#include "IrcReply.h"

#include <algorithm>
#include <cassert>
#include <locale>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
using namespace LibIrc;

#define SUCCESS 0
#define MAX_PARAMETERS_COUNT 15

const char* WSAFormatError(const int errorCode);

const std::string toUpperCase(const std::string str) {
    std::string tmp = str;
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
    return tmp;
}

const int getNumericUserMode(const std::vector<char> modes) {
    int value = 0;
    if (modes.empty()) {
        return value;
    }
    if (std::find(modes.begin(), modes.end(), 'w') != modes.end()) {
        value |= 0x02;
    }
    if (std::find(modes.begin(), modes.end(), 'i') != modes.end()) {
        value |= 0x04;
    }
    return value;
}

IrcClient::IrcClient() {
    auto startupResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (startupResult != 0) {
        printf("Error: %s\n", WSAFormatError(startupResult));
        return;
    }
}

IrcClient::~IrcClient() {
    this->users.clear();
    this->servers.clear();

    if (this->socket != INVALID_SOCKET) {
        if (::closesocket(this->socket) != SUCCESS) {
            printf("Error: %s\n", WSAFormatError(::WSAGetLastError()));
        }
    }

    ::WSACleanup();
}

void IrcClient::connect(const string hostName, const int port,
                        const IrcRegistrationInfo registrationInfo) {
    this->hostName = hostName;
    this->port = port;
    this->registrationInfo = registrationInfo;

    struct addrinfo* addressInfo;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET | AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int getAddrInfoResult =
        ::getaddrinfo(hostName.c_str(), to_string(port).c_str(), &hints, &addressInfo);
    if (getAddrInfoResult != SUCCESS) {
        printf("Error: %s\n", gai_strerror(getAddrInfoResult));
        ::WSACleanup();
        return;
    }

    this->socket =
        ::socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
    if (this->socket == INVALID_SOCKET) {
        printf("Error: %s\n", WSAFormatError(::WSAGetLastError()));
        freeaddrinfo(&hints);
        ::WSACleanup();
        return;
    }

    int connectResult = ::connect(this->socket, addressInfo->ai_addr, (int)addressInfo->ai_addrlen);
    if (connectResult == SOCKET_ERROR) {
        printf("Error: %s\n", WSAFormatError(::WSAGetLastError()));
        if (::closesocket(this->socket) != SUCCESS) {
            printf("Error: %s\n", WSAFormatError(::WSAGetLastError()));
        }
        ::WSACleanup();
        return;
    }

    this->connected();
    this->listen();
}

void IrcClient::connected() {
    if (!registrationInfo.password.empty()) {
        this->sendMessagePassword(registrationInfo.password);
    }
    this->sendMessageNick(registrationInfo.nickName);
    this->sendMessageUser(registrationInfo.userName, registrationInfo.realName,
                          registrationInfo.userModes);

    auto localUser = new IrcLocalUser();
    localUser->nickName = registrationInfo.nickName;
    localUser->userName = registrationInfo.userName;

    this->localUser = localUser;
    this->users.push_back(localUser);
}

void IrcClient::listen(const string remainder) {
    const int receiveBufferLength = 256;
    char receiveBuffer[receiveBufferLength];
    int bytesRead;

    bytesRead = ::recv(this->socket, receiveBuffer, receiveBufferLength, 0);
    if (bytesRead > 0) {
        auto receivedMessage = string(receiveBuffer, bytesRead);

        stringstream lineStream;
        lineStream << remainder;
        lineStream << receivedMessage;

        string line;
        while (getline(lineStream, line)) {
            if (lineStream.eof()) {
                listen(line); // String was incomplete line, prepend to next read.
                return;
            }

            if (line.empty()) {
                return;
            }

            // Remove \r as std::getline only splits on \n. IRC always uses
            // \r\n.
            line = line.substr(0, line.length() - 1);

            this->parseMessage(line);
        }

        this->listen();
    } else if (bytesRead == 0) {
        printf("Connection closed\r\n");
    } else {
        printf("Error: %s\n", WSAFormatError(::WSAGetLastError()));
    }
}

void IrcClient::sendRawMessage(const string message) {
    auto formattedMessage = message + "\r\n";
    auto buffer = formattedMessage.c_str();
    auto result = ::send(this->socket, buffer, (int)strlen(buffer), 0);
    if (result == SOCKET_ERROR) {
        printf("Error: %s\n", WSAFormatError(::WSAGetLastError()));
        if (::closesocket(this->socket) != SUCCESS) {
            printf("Error: %s\n", WSAFormatError(::WSAGetLastError()));
        }
        ::WSACleanup();
        return;
    }
}

/**
 * The extracted message is parsed into the components <prefix>,
 * <command> and list of parameters (<params>).
 *
 *  The Augmented BNF representation for this is:
 *
 *  message    =  [ ":" prefix SPACE ] command [ params ] crlf
 *  prefix     =  servername / ( nickname [ [ "!" user ] "@" host ] )
 *  command    =  1*letter / 3digit
 *  params     =  *14( SPACE middle ) [ SPACE ":" trailing ]
 *             =/ 14( SPACE middle ) [ SPACE [ ":" ] trailing ]
 *
 *  nospcrlfcl =  %x01-09 / %x0B-0C / %x0E-1F / %x21-39 / %x3B-FF
 *                   ; any octet except NUL, CR, LF, " " and ":"
 *  middle     =  nospcrlfcl *( ":" / nospcrlfcl )
 *  trailing   =  *( ":" / " " / nospcrlfcl )
 *
 *  SPACE      =  %x20        ; space character
 *  crlf       =  %x0D %x0A   ; "carriage return" "linefeed"
 *
 * Most protocol messages specify additional semantics and syntax for
 * the extracted parameter strings dictated by their position in the
 * list.  For example, many server commands will assume that the first
 * parameter after the command is the list of targets, which can be
 * described with:
 *
 *  target     =  nickname / server
 *  msgtarget  =  msgto *( "," msgto )
 *  msgto      =  channel / ( user [ "%" host ] "@" servername )
 *  msgto      =/ ( user "%" host ) / targetmask
 *  msgto      =/ nickname / ( nickname "!" user "@" host )
 *  channel    =  ( "#" / "+" / ( "!" channelid ) / "&" ) chanstring
 *                [ ":" chanstring ]
 *  servername =  hostname
 *  host       =  hostname / hostaddr
 *  hostname   =  shortname *( "." shortname )
 *  shortname  =  ( letter / digit ) *( letter / digit / "-" )
 *                *( letter / digit )
 *                  ; as specified in RFC 1123 [HNAME]
 *  hostaddr   =  ip4addr / ip6addr
 *  ip4addr    =  1*3digit "." 1*3digit "." 1*3digit "." 1*3digit
 *  ip6addr    =  1*hexdigit 7( ":" 1*hexdigit )
 *  ip6addr    =/ "0:0:0:0:0:" ( "0" / "FFFF" ) ":" ip4addr
 *  nickname   =  ( letter / special ) *8( letter / digit / special / "-" )
 *  targetmask =  ( "$" / "#" ) mask
 *                  ; see details on allowed masks in section 3.3.1
 *  chanstring =  %x01-07 / %x08-09 / %x0B-0C / %x0E-1F / %x21-2B
 *  chanstring =/ %x2D-39 / %x3B-FF
 *                  ; any octet except NUL, BELL, CR, LF, " ", "," and ":"
 *  channelid  = 5( %x41-5A / digit )   ; 5( A-Z / 0-9 )
 *
 * Other parameter syntaxes are:
 *
 *   user       =  1*( %x01-09 / %x0B-0C / %x0E-1F / %x21-3F / %x41-FF )
 *                   ; any octet except NUL, CR, LF, " " and "@"
 *   key        =  1*23( %x01-05 / %x07-08 / %x0C / %x0E-1F / %x21-7F )
 *                   ; any 7-bit US_ASCII character,
 *                   ; except NUL, CR, LF, FF, h/v TABs, and " "
 *   letter     =  %x41-5A / %x61-7A       ; A-Z / a-z
 *   digit      =  %x30-39                 ; 0-9
 *   hexdigit   =  digit / "A" / "B" / "C" / "D" / "E" / "F"
 *   special    =  %x5B-60 / %x7B-7D
 *                    ; "[", "]", "\", "`", "_", "^", "{", "|", "}"*
 */
void IrcClient::parseMessage(const string line) {
    string prefix;
    string lineAfterPrefix;

    if (line[0] == ':') {
        auto firstSpaceIndex = line.find(" ");
        prefix = line.substr(1, firstSpaceIndex - 1);
        lineAfterPrefix = line.substr(firstSpaceIndex + 1);
    } else {
        lineAfterPrefix = line;
    }

    size_t spaceIndex = lineAfterPrefix.find(" ");
    if (spaceIndex == std::string::npos) {
        // This is unexpected, but will cause a index-out-of-range assertion
        // if invalid input is provided to parseMessage(..)
        return;
    }

    string command = lineAfterPrefix.substr(0, spaceIndex);
    string paramsLine = lineAfterPrefix.substr(command.length() + 1);
    size_t paramsLineLength = paramsLine.length();

    vector<string> parameters;
    int paramStartIndex = -1;
    size_t paramEndIndex = std::string::npos;

    size_t lineColonIndex = paramsLine.find(" :");
    if (lineColonIndex == std::string::npos && paramsLine.find(":") != 0) {
        lineColonIndex = paramsLineLength;
    }

    for (int i = 0; i < MAX_PARAMETERS_COUNT; i++) {
        paramStartIndex = (int)paramEndIndex + 1;
        paramEndIndex = paramsLine.find(' ', paramStartIndex);

        if (paramEndIndex == std::string::npos) {
            paramEndIndex = paramsLineLength;
        }

        if (lineColonIndex == std::string::npos || paramEndIndex > lineColonIndex) {
            paramStartIndex++;
            paramEndIndex = paramsLineLength;
        }

        parameters.push_back(paramsLine.substr(paramStartIndex, paramEndIndex - paramStartIndex));

        if (paramEndIndex == paramsLineLength) {
            break;
        }
    }

    IrcMessage message;
    message.client = this;
    message.prefix = prefix;
    message.command = toUpperCase(command);
    message.parameters = parameters;
    message.source = this->getSourceFromPrefix(prefix);
    message.raw = line;

    this->processMessage(message);
}

void IrcClient::processMessage(const IrcMessage message) {
    if (message.command == CMD_PING) {
        processMessagePing(message);
        return;
    }

    this->emit("message", message);
}

void IrcClient::writeMessage(const string prefix, const string command,
                             const vector<string> parameters) {
    stringstream message;

    if (!prefix.empty()) {
        message << ": " << prefix << " ";
    }

    message << command;

    size_t n = parameters.size();
    for (int i = 0; i < n - 1; i++) {
        message << " " << parameters[i];
    }

    if (n > 0) {
        message << " :" << parameters[n - 1];
    }

    message << "\r\n";

    this->writeMessage(message.str());
}

void IrcClient::writeMessage(const string message) {
    if (message.empty()) {
        return;
    }

    auto buffer = message.c_str();

    int sendResult = ::send(this->socket, buffer, (int)strlen(buffer), 0);
    if (sendResult == SOCKET_ERROR) {
        printf("Error: %s\n", WSAFormatError(::WSAGetLastError()));
        if (::closesocket(this->socket) != SUCCESS) {
            printf("Error: %s\n", WSAFormatError(::WSAGetLastError()));
        }
        ::WSACleanup();
        return;
    }
}

// - Message Sending

void IrcClient::sendMessagePassword(const string password) {
    this->writeMessage("", "PASS", { password });
}

void IrcClient::sendMessageNick(const string nickName) {
    this->writeMessage("", "NICK", { nickName });
}

void IrcClient::sendMessageUser(const string userName, const string realName,
                                const vector<char> userModes) {
    int numericUserMode = getNumericUserMode(userModes);
    this->writeMessage("", "USER", { userName, to_string(numericUserMode), "*", realName });
}

void IrcClient::sendMessagePong(const string ping) {
    this->writeMessage("", "PONG", { ping });
}

// - Message Processing

void IrcClient::processMessagePing(const IrcMessage message) {
    assert(message.parameters.size() >= 1);
    this->sendMessagePong(message.parameters[0]);
}

// - Utils

IrcMessageSource* IrcClient::getSourceFromPrefix(const string prefix) {
    auto dotIndex = prefix.find('.') + 1;
    auto bangIndex = prefix.find('!') + 1;
    auto atIndex = prefix.find('@', bangIndex) + 1;

    if (bangIndex > 0) {
        auto nickName = prefix.substr(0, bangIndex - 1);
        auto user = this->getUserFromNickName(nickName);
        if (atIndex > 0) {
            user->userName = prefix.substr(bangIndex, atIndex - 1);
            user->hostName = prefix.substr(atIndex);
        } else {
            user->userName = prefix.substr(bangIndex);
        }
        return user;
    } else if (atIndex > 0) {
        auto nickName = prefix.substr(0, atIndex - 1);
        auto user = this->getUserFromNickName(nickName);
        user->hostName = prefix.substr(atIndex);
        return user;
    } else if (dotIndex > 0) {
        return this->getServerFromHostName(prefix);
    } else {
        return this->getUserFromNickName(prefix);
    }
}

IrcUser* IrcClient::getUserFromNickName(const string nickName) {
    auto user = find_if(this->users.begin(), this->users.end(), [&nickName](const IrcUser* obj) {
        return _stricmp(obj->nickName.c_str(), nickName.c_str()) == 0;
    });

    if (user != this->users.end()) {
        return *user;
    }

    auto newUser = new IrcUser();
    newUser->nickName = nickName;

    this->users.push_back(newUser);

    return newUser;
}

IrcServer* IrcClient::getServerFromHostName(const string hostName) {
    auto server =
        find_if(this->servers.begin(), this->servers.end(), [&hostName](const IrcServer* obj) {
            return _stricmp(obj->hostName.c_str(), hostName.c_str());
        });

    if (server != this->servers.end()) {
        return *server;
    }

    auto newServer = new IrcServer();
    newServer->hostName = hostName;

    this->servers.push_back(newServer);

    return newServer;
}

const char* WSAFormatError(const int errorCode) {
    LPSTR errString;

    int size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM, // windows internal message table
                             0,         // 0, since source is internal message table
                             errorCode, // error code returned by WSAGetLastError()
                             0,         // 0, auto-determine which language to use.
                             (LPSTR)&errString,
                             0,  // buffer minimum size.
                             0); // 0, since getting message from system tables

    if (size == 0) {
        return ("Unknown Error Code: " + to_string(errorCode)).c_str();
    }

    return errString;
}
