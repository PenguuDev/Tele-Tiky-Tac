#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <thread>
#include <iostream>

class Client
{
public:
	struct Settings
	{
		std::string ip;
		std::string port;
		bool debugMessages;
		bool alwaysConnect;
		int secondsTimeout;
	};

private:
	SOCKET socket;
	bool connected;
	bool checkConnection; // DONT TOUCH THIS VARIABLE
	Settings settings;
	std::thread clientThread;

public:
	Client(Settings newSettings);
	~Client();

	bool Connect();
	void Disconnect();

	bool Send(std::string message);
	std::string Receive();
	std::string Communicate(std::string message);

	bool IsConnected();
};