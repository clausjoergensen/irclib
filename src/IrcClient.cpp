// Copyright (c) 2018 Claus Jørgensen
#include "IrcClient.h"
#include <stdexcept>
#include <locale>
#include <algorithm>
#include <cassert>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
using namespace LibIrc;

#define MAX_PARAMETERS_COUNT 15

inline std::string toUpperCase(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

inline std::string toLowerCase(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

IrcClient::IrcClient()
{
	auto startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (startupResult != 0) {
		printf("WSAStartup failed: %d\n", startupResult);
		return;
	}
}

IrcClient::~IrcClient()
{
	if (this->socket != INVALID_SOCKET) {
		closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	WSACleanup();
}

void IrcClient::connect(string hostName, int port, IrcRegistrationInfo registrationInfo)
{
	this->hostName = hostName;
	this->port = port;
	this->registrationInfo = registrationInfo;

	struct addrinfo *addressInfo = NULL, *networkAddress = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	INT result;

	result = ::getaddrinfo(hostName.c_str(), to_string(port).c_str(), &hints, &addressInfo);
	if (result != 0) {
		printf("getaddrinfo failed: %d\n", result);
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

	result = ::connect(this->socket, networkAddress->ai_addr, (int)networkAddress->ai_addrlen);
	if (result == SOCKET_ERROR) {
		closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	this->sendMessageNick(registrationInfo.nickName);
	this->sendMessageUser(registrationInfo.userName, registrationInfo.realName);

	const int receiveBufferLength = 512;
	char receiveBuffer[receiveBufferLength];

	int bytesRead = 0;
	do {
		bytesRead = ::recv(this->socket, receiveBuffer, receiveBufferLength, 0);
		if (bytesRead > 0) {
			stringstream ss(string(receiveBuffer, bytesRead));
			string line;
			while (getline(ss, line))
			{
				if (line.empty())
				{
					continue;
				}

				// Remove \r
				line = line.substr(0, line.length() - 1);

				this->parseMessage(line);
			}
		}
		else if (bytesRead == 0) {
			std::cout << "Connection closed\r\n";
		}
		else {
			printf("recv failed: %d\n", WSAGetLastError());
		}
	} while (bytesRead > 0);
}

void IrcClient::sendRawMessage(string message)
{
	auto buffer = message.c_str();
	auto result = ::send(this->socket, buffer, (int)strlen(buffer), 0);
	if (result == SOCKET_ERROR) {
		printf("Send failed: %d\n", WSAGetLastError());
		closesocket(this->socket);
		WSACleanup();
		return;
	} else {
		printf("Sending: '%s'", buffer);
	}
}

void IrcClient::parseMessage(string line)
{
	std::cout << line << "\r\n";

	string prefix;
	string lineAfterPrefix;

	if (line[0] == ':')
	{
		auto firstSpaceIndex = line.find(" ");
		prefix = line.substr(1, firstSpaceIndex - 1);
		lineAfterPrefix = line.substr(firstSpaceIndex + 1);
	}
	else
	{
		lineAfterPrefix = line;
	}

	int spaceIndex = lineAfterPrefix.find(" ");
	auto command = lineAfterPrefix.substr(0, spaceIndex);
	auto paramsLine = lineAfterPrefix.substr(command.length() + 1);
	int paramsLineLength = paramsLine.length();

	vector<string> parameters;
	int paramStartIndex = -1;
	int paramEndIndex = -1;

	int lineColonIndex = paramsLine.find(" :");
	if (lineColonIndex == -1 && paramsLine.find(":") != 0)
	{
		lineColonIndex = paramsLineLength;
	}
	else
	{
		lineColonIndex = -1;
	}

	for (int i = 0; i < MAX_PARAMETERS_COUNT; i++)
	{
		paramStartIndex = paramEndIndex + 1;
		paramEndIndex = paramsLine.find(' ', paramStartIndex);

		if (paramEndIndex == -1) {
			paramEndIndex = paramsLineLength;
		}

		if (paramEndIndex > lineColonIndex)
		{
			paramStartIndex++;
			paramEndIndex = paramsLineLength;
		}

		parameters.push_back(paramsLine.substr(paramStartIndex, paramEndIndex - paramStartIndex));

		if (paramEndIndex == paramsLineLength)
		{
			break;
		}
	}

	IrcMessage message;
	message.client = this;
	message.prefix = prefix;
	message.command = toUpperCase(command);
	message.parameters = parameters;
	message.source = this->getSourceFromPrefix(prefix);

	this->processMessage(message);
}

void IrcClient::processMessage(IrcMessage message)
{
	if (message.command == "PING")
	{
		this->sendMessagePong(message.parameters[0]);
	}
	else if (message.command == "001")
	{

	}
}

void IrcClient::writeMessage(string prefix, string command, vector<string> parameters)
{
	stringstream ss;

	if (!prefix.empty())
	{
		ss << ": " << prefix << " ";
	}

	ss << command;

	int n = parameters.size();
	for (int i = 0; i < n - 1; i++)
	{
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
	else
	{
		printf("%s", buffer);
	}
}

void IrcClient::sendMessageNick(string nickName) {
	this->writeMessage("", "NICK", { nickName });
}

void IrcClient::sendMessageUser(string userName, string realName)
{
	this->writeMessage("", "USER", { userName, "0", "*", realName });
}

void IrcClient::sendMessagePong(string ping)
{
	this->writeMessage("", "PONG", { ping });
}

IrcMessageSource* IrcClient::getSourceFromPrefix(string prefix)
{
	auto dotIdx = prefix.find('.') + 1;
	auto bangIdx = prefix.find('!') + 1;
	auto atIdx = prefix.find('@', bangIdx) + 1;

	if (bangIdx > 0)
	{
		auto nickName = prefix.substr(0, bangIdx - 1);
		auto user = this->getUserFromNickName(nickName);
		if (atIdx > 0) {
			user->userName = prefix.substr(bangIdx, atIdx - 1);
			user->hostName = prefix.substr(atIdx);
		}
		else
		{
			user->userName = prefix.substr(bangIdx);
		}
		return user;
	}
	else if (atIdx > 0)
	{
		auto nickName = prefix.substr(0, atIdx - 1);
		auto user = this->getUserFromNickName(nickName);
		user->hostName = prefix.substr(atIdx);
		return user;
	}
	else if (dotIdx > 0)
	{
		return this->getServerFromHostName(prefix);
	}
	else
	{
		return this->getUserFromNickName(prefix);
	}
}

IrcUser* IrcClient::getUserFromNickName(string nickName)
{
	auto user = find_if(this->users.begin(), this->users.end(), [&nickName](const IrcUser* obj) {
		return obj->nickName == nickName;
	});

	if (user != this->users.end())
	{
		return *user;
	}

	auto newUser = new IrcUser();
	newUser->nickName = nickName;

	this->users.push_back(newUser);

	return newUser;
}

IrcServer* IrcClient::getServerFromHostName(string hostName)
{
	auto server = find_if(this->servers.begin(), this->servers.end(), [&hostName](const IrcServer* obj) {
		return obj->hostName == hostName;
	});

	if (server != this->servers.end())
	{
		return *server;
	}

	auto newServer = new IrcServer();
	newServer->hostName = hostName;

	this->servers.push_back(newServer);

	return newServer;
}