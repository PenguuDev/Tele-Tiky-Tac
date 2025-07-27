#include "network.h"

Client::Client(Settings newSettings)
{
	socket = INVALID_SOCKET;
	connected = false;
	settings = newSettings;

	clientThread = std::thread([this]() { Connect(); });
	//Connect();
}

Client::~Client()
{
	clientThread.join();
	Disconnect();
}

bool Client::Connect()
{
	std::thread([this]()
		{
			while (checkConnection)
			{
				char buffer;
				int result = recv(socket, &buffer, 1, MSG_PEEK);

				if (result == 0)
				{
					connected = false;
				}
				else if (result == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					if (error != WSAEWOULDBLOCK)
					{
						connected = false;
					}
					else
					{
						connected = true;
					}
				}
				else
				{
					connected = true;
				}

				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}).detach();

	while (true)
	{
		if (connected)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

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

		int connectAttempt = 0;
		while (true)
		{
			if (connectAttempt > settings.secondsTimeout && settings.secondsTimeout != -1)
			{
				connected = false;
				throw std::runtime_error("failed connecting to the server");
				break;
			}

			status = connect(socket, result->ai_addr, (int)result->ai_addrlen);
			if (status == SOCKET_ERROR)
			{
				connectAttempt++;
				if (settings.secondsTimeout == -1)
				{
					std::cout << "Couldn't connect to server. Retrying\n";
				}
				else
				{
					std::cout << "Couldn't connect to server. This was attempt: " << connectAttempt << " / " << settings.secondsTimeout << "\n";
				}
				Sleep(1000);
				continue;
			}
			connected = true;
			break;
		}

		freeaddrinfo(result);

		if (settings.debugMessages)
		{
			std::cout << "Connected to server.\n";
		}

		//
		if (!settings.alwaysConnect)
		{
			break;
		}
	}

	return true;
}
void Client::Disconnect()
{
	if (connected)
	{
		shutdown(socket, SD_BOTH);
		closesocket(socket);
		WSACleanup();
		connected = false;
		checkConnection = false; // Stop checking if we are connected

		if (settings.debugMessages)
		{
			std::cout << "Disconnected.\n";
		}
	}
}

bool Client::Send(std::string message)
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
std::string Client::Receive()
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
std::string Client::Communicate(std::string message)
{
	Send(message);
	return Receive();
}

bool Client::IsConnected()
{
	return connected;
}