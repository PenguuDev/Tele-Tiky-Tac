#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <thread>
#include <iostream>

class Server
{
public:
	struct Settings
	{
		std::string port;
		bool debugMessages;
	};

	struct Client
	{
		SOCKET socket;
		size_t id;
		std::string name;
		bool connected;
	};

private:
	std::vector<Client> clients;
	SOCKET server;
	std::thread serverThread;

	bool running;
	Settings settings;

	void HandleConnection(SOCKET connection)
	{
		Client newClient;
		newClient.socket = connection;
		newClient.name = GetSocketRemoteName(connection);
		newClient.id = clients.size() + 1;
		newClient.connected = true;
		clients.push_back(newClient);

		std::cout << "New connection " << newClient.name << "\n";

		ClientHandler(newClient.id);
	}

	void ClientHandler(size_t clientId)
	{
		std::thread([this, clientId]()
			{
				while (running)
				{
					Client* client = FindClientById(clientId);
					if (client == nullptr || !client->connected)
					{
						break;
					}

					char buffer;
					int result = recv(client->socket, &buffer, 1, MSG_PEEK);

					if (result == 0 || (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK))
					{
						RemoveClient(*client, true);
						break;
					}

					Sleep(1300);
				}
			}
		).detach();
	}

public:
	Server(Settings newSettings)
	{
		running = false;
		server = INVALID_SOCKET;
		settings = newSettings;
		serverThread = std::thread([this]() { Start(); });
	}

	Server()
	{
		running = false;
		server = INVALID_SOCKET;
	}

	~Server()
	{
		Stop();
		if (serverThread.joinable()) serverThread.join();
	}

	bool Start()
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
		hints.ai_flags = AI_PASSIVE;

		status = getaddrinfo(0, settings.port.c_str(), &hints, &result);
		if (status != 0)
		{
			throw std::runtime_error("getaddrinfo failed with status code " + std::to_string(status));
			WSACleanup();
			return false;
		}

		server = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (server == INVALID_SOCKET)
		{
			throw std::runtime_error("socket failed with status code " + std::to_string(WSAGetLastError()));
			freeaddrinfo(result);
			WSACleanup();
			return false;
		}

		status = bind(server, result->ai_addr, (int)result->ai_addrlen);
		if (status != 0)
		{
			throw std::runtime_error("bind failed with status code " + std::to_string(WSAGetLastError()));
			freeaddrinfo(result);
			closesocket(server);
			WSACleanup();
			return false;
		}

		freeaddrinfo(result);

		status = listen(server, SOMAXCONN);
		if (status == SOCKET_ERROR)
		{
			throw std::runtime_error("listen failed with error: " + std::to_string(WSAGetLastError()));
			closesocket(server);
			WSACleanup();
			return false;
		}

		if (settings.debugMessages)
		{
			std::cout << "Server is listening for incoming connections.\n";
		}

		running = true;

		while (running)
		{
			SOCKET clientSocket = accept(server, 0, 0);
			if (clientSocket == INVALID_SOCKET)
			{
				continue;
			}

			HandleConnection(clientSocket);
		}

		return true;
	}

	bool Stop()
	{
		running = false;
		KickAll();
		closesocket(server);
		WSACleanup();
		return true;
	}

	void RemoveClient(Client client, bool serverMessage = false)
	{
		for (auto it = clients.begin(); it != clients.end(); ++it)
		{
			if (it->id == client.id)
			{
				if (settings.debugMessages || serverMessage)
				{
					std::cout << "Client disconnected " << client.name << "\n";
				}

				shutdown(it->socket, SD_BOTH);
				closesocket(it->socket);
				clients.erase(it);
				break;
			}
		}
	}

	void KickAll()
	{
		while (!clients.empty())
		{
			RemoveClient(clients.back());
		}
	}

	void Broadcast(std::string message)
	{
		for (auto& client : clients)
		{
			Send(client, message);
		}
	}

	bool Send(Client client, std::string message)
	{
		if (!client.connected)
		{
			return true;
		}

		size_t sizeSent = 0;
		size_t messageSize = message.size() + 1;

		while (sizeSent < messageSize)
		{
			int sent = send(client.socket, message.c_str(), messageSize, 0);
			if (sent == SOCKET_ERROR)
			{
				return false;
			}
			sizeSent += sent;
		}
		return true;
	}

	std::string Receive(Client& client)
	{
		if (!client.connected)
		{
			return "";
		}

		std::string result;
		char buffer[1024];

		while (true)
		{
			int received = recv(client.socket, buffer, sizeof(buffer), 0);
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

	std::string Communicate(Client client, std::string message)
	{
		Send(client, message);
		return Receive(client);
	}

	bool IsClientConnected(Client client)
	{
		if (client.connected == false)
		{
			return false;
		}
		if (client.id > clients.size()) // best one
		{
			return false;
		}
		Client* found = FindClientById(client.id);
		if (found == nullptr)
		{
			return false;
		}
		return found->connected;
	}

	SOCKET GetServer()
	{
		return server;
	}
	Client* FindClientById(size_t id)
	{
		for (auto& client : clients)
		{
			if (client.id == id)
			{
				return &client;
			}
		}
		return nullptr;
	}
	std::vector<Client> GetClients()
	{
		return clients;
	}
	Client GetClient(size_t iteratorNumber)
	{
		if (iteratorNumber >= clients.size())
		{
			return {};
		}
		return clients[iteratorNumber];
	}

	static std::string GetSocketRemoteName(SOCKET s)
	{
		sockaddr_in addr;
		int addrSize = sizeof(addr);
		if (getpeername(s, (sockaddr*)&addr, &addrSize) == 0)
		{
			char ipStr[INET_ADDRSTRLEN] = { 0 };
			inet_ntop(AF_INET, &(addr.sin_addr), ipStr, INET_ADDRSTRLEN);

			unsigned short port = ntohs(addr.sin_port);

			return std::string(ipStr) + ":" + std::to_string(port);
		}
		return "Unknown";
	}
};

int HasPlayerLeft(Server& server, Server::Client& player1, Server::Client& player2)
{
	int status = 0;

	if (server.IsClientConnected(player1))
	{
		status++;
	}
	if (server.IsClientConnected(player2))
	{
		status++;
	}
	return status;
}

std::string player1Query;
std::string player2Query;

void ReceiveMsgThread(Server& server, Server::Client& player1, Server::Client& player2)
{
	std::thread([&server, &player1]()
		{
			while (true)
			{
				player1Query = server.Receive(player1);
			}
		}
	).detach();
	std::thread([&server, &player2]()
		{
			while (true)
			{
				player1Query = server.Receive(player2);
			}
		}
	).detach();
}

int main()
{
	// Initializing our server
	Server::Settings settings;
	settings.port = "3709";
	settings.debugMessages = true;
	Server server(settings);

	// Server loop
	bool hasGameStarted = false;
	bool hasRoundStarted = false;
	bool hasToldClientToStart = false;

	Server::Client player1;
	Server::Client player2;
	ReceiveMsgThread(server, player1, player2);
	while (true)
	{
		player1 = server.GetClient(0);
		player2 = server.GetClient(1);
		if (!player1.connected || !player2.connected)
		{
			server.Send(player1, "WaitScreen");
			hasToldClientToStart = false;
			Sleep(1000);
		}
		else if (player1.connected && player2.connected)
		{
			for (int waitTime = 3; waitTime > 0; waitTime--)
			{
				int hasPlayerLeft = HasPlayerLeft(server, player1, player2);
				if (hasPlayerLeft != 2)
				{
					break;
				}
				else
				{
					server.Broadcast("StartIn" + std::to_string(waitTime));
					Sleep(1000);
				}
			}
			//while (true)
			//{
			//	if (!hasToldClientToStart)
			//	{
			//		server.Broadcast("Start");
			//		hasToldClientToStart = true;
			//	}
			//	int hasPlayerLeft = HasPlayerLeft(server, player1, player2);
			//	if (hasPlayerLeft != 2)
			//	{
			//		break;
			//	}
			//	else
			//	{
			//		// Game using receive query
			//	}
			//}
		}
	}
}