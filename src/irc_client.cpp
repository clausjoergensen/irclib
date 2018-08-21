// This code is licensed under MIT license (see LICENSE.txt for details)
#include "pch.h"

#include "irc_client.h"
#include "irc_commands.h"
#include "irc_errors.h"
#include "irc_replies.h"

using namespace std;
using namespace irclib;

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
    auto startup_result = ::WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (startup_result != 0) {
        this->emit(NETWORK_ERROR, WSAFormatError(startup_result));
        return;
    }
}

IrcClient::~IrcClient() {
    this->users.clear();
    this->servers.clear();

    if (this->socket != INVALID_SOCKET) {
        if (::closesocket(this->socket) != SUCCESS) {
            this->emit(NETWORK_ERROR, WSAFormatError(::WSAGetLastError()));
        }
    }

    ::WSACleanup();
}

void IrcClient::connect(const string hostname, const int port,
                        const IrcRegistrationInfo registration_info) {
    this->hostname = hostname;
    this->port = port;
    this->registration_info = registration_info;

    struct addrinfo* addrinfo;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET | AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int getaddrinfo_result =
        ::getaddrinfo(hostname.c_str(), to_string(port).c_str(), &hints, &addrinfo);
    if (getaddrinfo_result != SUCCESS) {
        this->emit(NETWORK_ERROR, gai_strerror(getaddrinfo_result));
        ::WSACleanup();
        return;
    }

    this->socket = ::socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (this->socket == INVALID_SOCKET) {
        this->emit(NETWORK_ERROR, WSAFormatError(::WSAGetLastError()));
        freeaddrinfo(&hints);
        ::WSACleanup();
        return;
    }

    int connect_result = ::connect(this->socket, addrinfo->ai_addr, (int)addrinfo->ai_addrlen);
    if (connect_result == SOCKET_ERROR) {
        this->emit(NETWORK_ERROR, WSAFormatError(::WSAGetLastError()));
        if (::closesocket(this->socket) != SUCCESS) {
            this->emit(NETWORK_ERROR, WSAFormatError(::WSAGetLastError()));
        }
        ::WSACleanup();
        return;
    }

    this->connected();
    this->listen();
}

void IrcClient::connected() {
    if (!this->registration_info.password.empty()) {
        this->sendMessagePassword(this->registration_info.password);
    }
    this->sendMessageNick(this->registration_info.nickname);
    this->sendMessageUser(this->registration_info.username, 
                          this->registration_info.realname,
                          this->registration_info.user_modes);

    auto local_user = new IrcLocalUser(this->registration_info.nickname);
    local_user->username = this->registration_info.username;

    this->local_user = local_user;

    std::lock_guard<std::mutex> lock(mutex);

    this->users.push_back(local_user);
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

            // Remove \r as std::getline only splits on \n.
            // IRC always uses \r\n.
            line = line.substr(0, line.length() - 1);

            this->parseMessage(line);
        }

        this->listen();
    } else if (bytesRead == 0) {
        this->emit(NETWORK_ERROR, "Connection closed.");
    } else {
        this->emit(NETWORK_ERROR, WSAFormatError(::WSAGetLastError()));
    }
}

void IrcClient::sendRawMessage(const string message) {
    auto formattedMessage = message + "\r\n";
    auto buffer = formattedMessage.c_str();
    auto result = ::send(this->socket, buffer, (int)strlen(buffer), 0);
    if (result == SOCKET_ERROR) {
        this->emit(NETWORK_ERROR, WSAFormatError(::WSAGetLastError()));
        if (::closesocket(this->socket) != SUCCESS) {
            this->emit(NETWORK_ERROR, WSAFormatError(::WSAGetLastError()));
        }
        ::WSACleanup();
        return;
    }
}

void IrcClient::parseMessage(const string line) {
    // The extracted message is parsed into the components <prefix>,
    // <command> and list of parameters (<params>).
    //
    //  The Augmented BNF representation for this is:
    //
    //  message    =  [ ":" prefix SPACE ] command [ params ] crlf
    //  prefix     =  servername / ( nickname [ [ "!" user ] "@" host ] )
    //  command    =  1*letter / 3digit
    //  params     =  *14( SPACE middle ) [ SPACE ":" trailing ]
    //             =/ 14( SPACE middle ) [ SPACE [ ":" ] trailing ]
    //
    //  nospcrlfcl =  %x01-09 / %x0B-0C / %x0E-1F / %x21-39 / %x3B-FF
    //                   ; any octet except NUL, CR, LF, " " and ":"
    //  middle     =  nospcrlfcl *( ":" / nospcrlfcl )
    //  trailing   =  *( ":" / " " / nospcrlfcl )
    //
    //  SPACE      =  %x20        ; space character
    //  crlf       =  %x0D %x0A   ; "carriage return" "linefeed"
    //
    // Most protocol messages specify additional semantics and syntax for
    // the extracted parameter strings dictated by their position in the
    // list.  For example, many server commands will assume that the first
    // parameter after the command is the list of targets, which can be
    // described with:
    //
    //  target     =  nickname / server
    //  msgtarget  =  msgto *( "," msgto )
    //  msgto      =  channel / ( user [ "%" host ] "@" servername )
    //  msgto      =/ ( user "%" host ) / targetmask
    //  msgto      =/ nickname / ( nickname "!" user "@" host )
    //  channel    =  ( "#" / "+" / ( "!" channelid ) / "&" ) chanstring
    //                [ ":" chanstring ]
    //  servername =  hostname
    //  host       =  hostname / hostaddr
    //  hostname   =  shortname *( "." shortname )
    //  shortname  =  ( letter / digit ) *( letter / digit / "-" )
    //                *( letter / digit )
    //                  ; as specified in RFC 1123 [HNAME]
    //  hostaddr   =  ip4addr / ip6addr
    //  ip4addr    =  1*3digit "." 1*3digit "." 1*3digit "." 1*3digit
    //  ip6addr    =  1*hexdigit 7( ":" 1*hexdigit )
    //  ip6addr    =/ "0:0:0:0:0:" ( "0" / "FFFF" ) ":" ip4addr
    //  nickname   =  ( letter / special ) *8( letter / digit / special / "-" )
    //  targetmask =  ( "$" / "#" ) mask
    //                  ; see details on allowed masks in section 3.3.1
    //  chanstring =  %x01-07 / %x08-09 / %x0B-0C / %x0E-1F / %x21-2B
    //  chanstring =/ %x2D-39 / %x3B-FF
    //                  ; any octet except NUL, BELL, CR, LF, " ", "," and ":"
    //  channelid  = 5( %x41-5A / digit )   ; 5( A-Z / 0-9 )
    //
    // Other parameter syntaxes are:
    //
    //   user       =  1*( %x01-09 / %x0B-0C / %x0E-1F / %x21-3F / %x41-FF )
    //                   ; any octet except NUL, CR, LF, " " and "@"
    //   key        =  1*23( %x01-05 / %x07-08 / %x0C / %x0E-1F / %x21-7F )
    //                   ; any 7-bit US_ASCII character,
    //                   ; except NUL, CR, LF, FF, h/v TABs, and " "
    //   letter     =  %x41-5A / %x61-7A       ; A-Z / a-z
    //   digit      =  %x30-39                 ; 0-9
    //   hexdigit   =  digit / "A" / "B" / "C" / "D" / "E" / "F"
    //   special    =  %x5B-60 / %x7B-7D
    //                    ; "[", "]", "\", "`", "_", "^", "{", "|", "}"*
    //

    string prefix;
    string line_after_prefix;

    if (line[0] == ':') {
        auto first_space_index = line.find(" ");
        prefix = line.substr(1, first_space_index - 1);
        line_after_prefix = line.substr(first_space_index + 1);
    } else {
        line_after_prefix = line;
    }

    size_t space_index = line_after_prefix.find(" ");
    if (space_index == std::string::npos) {
        // This is unexpected, but will cause a index-out-of-range assertion
        // if invalid input is provided to parseMessage(..)
        return;
    }

    string command = line_after_prefix.substr(0, space_index);
    string params_line = line_after_prefix.substr(command.length() + 1);
    size_t params_line_length = params_line.length();

    vector<string> parameters;
    int param_start_index = -1;
    size_t param_end_index = std::string::npos;

    size_t line_colon_index = params_line.find(" :");
    if (line_colon_index == std::string::npos && params_line.find(":") != 0) {
        line_colon_index = params_line_length;
    }

    for (int i = 0; i < MAX_PARAMETERS_COUNT; i++) {
        param_start_index = (int)param_end_index + 1;
        param_end_index = params_line.find(' ', param_start_index);

        if (param_end_index == std::string::npos) {
            param_end_index = params_line_length;
        }

        if (line_colon_index == std::string::npos || param_end_index > line_colon_index) {
            param_start_index++;
            param_end_index = params_line_length;
        }

        parameters.push_back(params_line.substr(param_start_index, param_end_index - param_start_index));

        if (param_end_index == params_line_length) {
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

    auto numeric_command = strtol(message.command.c_str(), nullptr, 10);
    if (numeric_command >= 400 && numeric_command <= 599) {
        this->emit(PROTOCOL_ERROR, message);
    } else {
        this->emit(message.command, message);
    }
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
        this->emit(NETWORK_ERROR, WSAFormatError(::WSAGetLastError()));
        if (::closesocket(this->socket) != SUCCESS) {
            this->emit(NETWORK_ERROR, WSAFormatError(::WSAGetLastError()));
        }
        ::WSACleanup();
        return;
    }
}

// - Message Sending

void IrcClient::sendMessagePassword(const string password) {
    this->writeMessage("", "PASS", { password });
}

void IrcClient::sendMessageNick(const string nickname) {
    this->writeMessage("", "NICK", { nickname });
}

void IrcClient::sendMessageUser(const string username, const string realname,
                                const vector<char> user_modes) {
    int numericUserMode = getNumericUserMode(user_modes);
    this->writeMessage("", "USER", { username, to_string(numericUserMode), "*", realname });
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
    auto dot_index = prefix.find('.') + 1;
    auto bang_index = prefix.find('!') + 1;
    auto at_index = prefix.find('@', bang_index) + 1;

    if (bang_index > 0) {
        auto nickName = prefix.substr(0, bang_index - 1);
        auto user = this->getUserFromNickName(nickName);
        if (at_index > 0) {
            user->username = prefix.substr(bang_index, at_index - 1);
            user->hostname = prefix.substr(at_index);
        } else {
            user->username = prefix.substr(bang_index);
        }
        return user;
    } else if (at_index > 0) {
        auto nickname = prefix.substr(0, at_index - 1);
        auto user = this->getUserFromNickName(nickname);
        user->hostname = prefix.substr(at_index);
        return user;
    } else if (dot_index > 0) {
        return this->getServerFromHostName(prefix);
    } else {
        return this->getUserFromNickName(prefix);
    }
}

IrcUser* IrcClient::getUserFromNickName(const string nickname) {
    std::lock_guard<std::mutex> lock(mutex);

    auto user = find_if(this->users.begin(), this->users.end(), [&nickname](const IrcUser* obj) {
        return _stricmp(obj->nickname.c_str(), nickname.c_str()) == 0;
    });

    if (user != this->users.end()) {
        return *user;
    }

    auto newUser = new IrcUser(nickname);
    this->users.push_back(newUser);

    return newUser;
}

IrcServer* IrcClient::getServerFromHostName(const string hostname) {
    std::lock_guard<std::mutex> lock(mutex);

    auto server =
        find_if(this->servers.begin(), this->servers.end(), [&hostname](const IrcServer* obj) {
            return _stricmp(obj->hostname.c_str(), hostname.c_str());
        });

    if (server != this->servers.end()) {
        return *server;
    }

    auto newServer = new IrcServer(hostname);
    this->servers.push_back(newServer);

    return newServer;
}

const char* WSAFormatError(const int error_code) {
    LPSTR error_string;

    int size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM, // windows internal message table
                             0,          // 0, since source is internal message table
                             error_code, // error code returned by WSAGetLastError()
                             0,          // 0, auto-determine which language to use.
                             (LPSTR)&error_string,
                             0,  // buffer minimum size.
                             0); // 0, since getting message from system tables

    if (size == 0) {
        return ("Unknown error code: " + to_string(error_code)).c_str();
    }

    return error_string;
}
