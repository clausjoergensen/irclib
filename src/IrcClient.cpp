// Copyright (c) 2018 Claus Jørgensen

#include "IrcClient.h"

#include <locale>
#include <algorithm>
#include <cassert>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
using namespace LibIrc;

#define SUCCESS 0
#define MAX_PARAMETERS_COUNT 15

inline std::string toUpperCase(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

inline std::string toLowerCase(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

int getNumericUserMode(std::vector<char> modes) {
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
	auto startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (startupResult != 0) {
		printf("WSAStartup failed: %d\n", startupResult);
		return;
	}
}

IrcClient::~IrcClient() {
	if (this->socket != INVALID_SOCKET) {
		closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	WSACleanup();
}

void IrcClient::connect(string hostName, int port, IrcRegistrationInfo registrationInfo) {
	this->hostName = hostName;
	this->port = port;
	this->registrationInfo = registrationInfo;

	// Create and Connect a WINSOCK socket.
	struct addrinfo *addressInfo = NULL, *networkAddress = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	INT getAddrInfoResult = ::getaddrinfo(hostName.c_str(), to_string(port).c_str(), &hints, &addressInfo);
	if (getAddrInfoResult != SUCCESS) {
		printf("getaddrinfo failed: %d\n", getAddrInfoResult);
		WSACleanup();
		return;
	}

	networkAddress = addressInfo;

	this->socket = ::socket(networkAddress->ai_family, networkAddress->ai_socktype, networkAddress->ai_protocol);
	if (this->socket == INVALID_SOCKET) {
		printf("Error at socket(): %d\n", WSAGetLastError());
		freeaddrinfo(&hints);
		WSACleanup();
		return;
	}

	INT connectResult = ::connect(this->socket, networkAddress->ai_addr, (int)networkAddress->ai_addrlen);
	if (connectResult == SOCKET_ERROR) {
		closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	// Once we're connected to the server, send the registration commands NICK and USER, and optionally PASSWORD.
	if (!registrationInfo.password.empty()) {
		this->sendMessagePassword(registrationInfo.password);
	}
	this->sendMessageNick(registrationInfo.nickName);
	this->sendMessageUser(registrationInfo.userName, registrationInfo.realName, registrationInfo.userModes);

	// Instantitate a local user.
	auto localUser = new IrcLocalUser();
	localUser->nickName = registrationInfo.nickName;
	localUser->userName = registrationInfo.userName;

	this->users.push_back(localUser);

	// Start listen for input
	const int receiveBufferLength = 512;
	char receiveBuffer[receiveBufferLength];
	int bytesRead;

	do {
		bytesRead = ::recv(this->socket, receiveBuffer, receiveBufferLength, 0);
		if (bytesRead > 0) {
			stringstream ss(string(receiveBuffer, bytesRead));
			string line;
			while (getline(ss, line)) {
				// Remove \r as std::getline only splits on \n. IRC always uses \r\n.
				line = line.substr(0, line.length() - 1);

				this->parseMessage(line);
			}
		} else if (bytesRead == 0) {
			printf("Connection closed\r\n");
		} else {
			printf("recv failed: %d\n", WSAGetLastError());
		}
	} while (bytesRead > 0);
}

void IrcClient::sendRawMessage(string message) {
	message = message + "\r\n";

	auto buffer = message.c_str();
	auto result = ::send(this->socket, buffer, (int)strlen(buffer), 0);
	if (result == SOCKET_ERROR) {
		printf("Send failed: %d\n", WSAGetLastError());
		closesocket(this->socket);
		WSACleanup();
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
void IrcClient::parseMessage(string line) {
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

void IrcClient::processMessage(IrcMessage message) {
	if (message.command == "PING") {
		this->sendMessagePong(message.parameters[0]);
	}

	if (this->onMessage != nullptr) {
		this->onMessage(message);
	}
}

void IrcClient::writeMessage(string prefix, string command, vector<string> parameters) {
	stringstream ss;

	if (!prefix.empty()) {
		ss << ": " << prefix << " ";
	}

	ss << command;

	size_t n = parameters.size();
	for (int i = 0; i < n - 1; i++) {
		ss << " " << parameters[i];
	}

	if (n > 0) {
		ss << " :" << parameters[n - 1];
	}

	ss << "\r\n";

	auto message = ss.str();
	auto buffer = message.c_str();

	auto result = ::send(this->socket, buffer, (int)strlen(buffer), 0);
	if (result == SOCKET_ERROR) {
		printf("Send failed: %d\n", WSAGetLastError());
		closesocket(this->socket);
		WSACleanup();
		return;
	}
}

void IrcClient::sendMessagePassword(string password) {
	this->writeMessage("", "PASS", { password });
}

void IrcClient::sendMessageNick(string nickName) {
	this->writeMessage("", "NICK", { nickName });
}

void IrcClient::sendMessageUser(string userName, string realName, vector<char> userModes) {
	int numericUserMode = getNumericUserMode(userModes);
	this->writeMessage("", "USER", { userName, to_string(numericUserMode), "*", realName });
}

void IrcClient::sendMessagePong(string ping) {
	this->writeMessage("", "PONG", { ping });
}

IrcMessageSource* IrcClient::getSourceFromPrefix(string prefix) {
	auto dotIdx = prefix.find('.') + 1;
	auto bangIdx = prefix.find('!') + 1;
	auto atIdx = prefix.find('@', bangIdx) + 1;

	if (bangIdx > 0) {
		auto nickName = prefix.substr(0, bangIdx - 1);
		auto user = this->getUserFromNickName(nickName);
		if (atIdx > 0) {
			user->userName = prefix.substr(bangIdx, atIdx - 1);
			user->hostName = prefix.substr(atIdx);
		} else {
			user->userName = prefix.substr(bangIdx);
		}
		return user;
	} else if (atIdx > 0) {
		auto nickName = prefix.substr(0, atIdx - 1);
		auto user = this->getUserFromNickName(nickName);
		user->hostName = prefix.substr(atIdx);
		return user;
	} else if (dotIdx > 0) {
		return this->getServerFromHostName(prefix);
	} else {
		return this->getUserFromNickName(prefix);
	}
}

IrcUser* IrcClient::getUserFromNickName(string nickName) {
	auto user = find_if(this->users.begin(), this->users.end(), [&nickName](const IrcUser* obj) {
		return toLowerCase(obj->nickName) == toLowerCase(nickName);
	});

	if (user != this->users.end()) {
		return *user;
	}

	auto newUser = new IrcUser();
	newUser->nickName = nickName;

	this->users.push_back(newUser);

	return newUser;
}

IrcServer* IrcClient::getServerFromHostName(string hostName) {
	auto server = find_if(this->servers.begin(), this->servers.end(), [&hostName](const IrcServer* obj) {
		return toLowerCase(obj->hostName) == toLowerCase(hostName);
	});

	if (server != this->servers.end()) {
		return *server;
	}

	auto newServer = new IrcServer();
	newServer->hostName = hostName;

	this->servers.push_back(newServer);

	return newServer;
}