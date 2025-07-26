#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <thread>
#include <iostream>

#pragma comment (lib, "Ws2_32.lib")

class Client
{
public:
	struct Settings
	{
		std::string ip;
		std::string port;
		bool debugMessages;
		size_t secondsTimeout;
	};

private:
	SOCKET socket;
	bool connected;
	Settings settings;

public:
	Client(Settings newSettings)
	{
		socket = INVALID_SOCKET;
		connected = false;
		settings = newSettings;

		Connect();
	}

	~Client()
	{
		Disconnect();
	}

	bool Connect()
	{
		WSADATA wsaData;
		int status = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (status != 0)
		{
			throw std::runtime_error("WSAStartup failed with status code " + std::to_string(status));
			return false;
		}

		struct addrinfo* result = nullptr;
		struct addrinfo hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		status = getaddrinfo(settings.ip.c_str(), settings.port.c_str(), &hints, &result);
		if (status != 0)
		{
			throw std::runtime_error("getaddrinfo failed with status code " + std::to_string(status));
			WSACleanup();
			return false;
		}

		socket = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (socket == INVALID_SOCKET)
		{
			throw std::runtime_error("socket failed with status code " + std::to_string(WSAGetLastError()));
			freeaddrinfo(result);
			WSACleanup();
			return false;
		}

		status = connect(socket, result->ai_addr, (int)result->ai_addrlen);
		if (status == SOCKET_ERROR)
		{
			throw std::runtime_error("connect failed with status code " + std::to_string(WSAGetLastError()));
			closesocket(socket);
			freeaddrinfo(result);
			WSACleanup();
			return false;
		}

		freeaddrinfo(result);
		connected = true;

		if (settings.debugMessages)
		{
			std::cout << "Connected to server.\n";
		}

		return true;
	}

	void Disconnect()
	{
		if (connected)
		{
			shutdown(socket, SD_BOTH);
			closesocket(socket);
			WSACleanup();
			connected = false;

			if (settings.debugMessages)
			{
				std::cout << "Disconnected.\n";
			}
		}
	}

	bool Send(std::string message)
	{
		if (!connected)
		{
			return false;
		}

		size_t sizeSent = 0;
		size_t messageSize = message.size() + 1;

		while (sizeSent < messageSize)
		{
			int sent = send(socket, message.c_str(), messageSize, 0);
			if (sent == SOCKET_ERROR)
			{
				return false;
			}
			sizeSent += sent;
		}
		return true;
	}

	std::string Receive()
	{
		if (!connected)
		{
			return "";
		}

		std::string result;
		char buffer[1024];

		while (true)
		{
			int received = recv(socket, buffer, sizeof(buffer), 0);
			if (received == SOCKET_ERROR || received == 0)
			{
				break;
			}

			result.append(buffer, received);

			if (received < sizeof(buffer))
			{
				break;
			}
		}

		return result;
	}

	std::string Communicate(std::string message)
	{
		Send(message);
		return Receive();
	}

	bool IsConnected()
	{
		return connected;
	}
};

int main()
{
	Client::Settings settings;
	settings.port = "3709";
	settings.ip = "127.0.0.1";
	settings.debugMessages = false;
	settings.secondsTimeout = 5;

	Client client(settings);
	while (true)
	{
		std::cout << client.Receive() << "\n";
	}
}