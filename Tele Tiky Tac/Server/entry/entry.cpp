#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <thread>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

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
		bool connected;
	};

private:
	std::vector<Client> clients;
	SOCKET serverSocket;
	bool running;

public:
	Server(Settings settings)
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		addrinfo hints{}, * result = nullptr;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;
		getaddrinfo(NULL, settings.port.c_str(), &hints, &result);

		serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		bind(serverSocket, result->ai_addr, (int)result->ai_addrlen);
		listen(serverSocket, SOMAXCONN);

		running = true;
		std::thread(&Server::Run, this).detach();
	}

	~Server()
	{
		running = false;
		closesocket(serverSocket);
		WSACleanup();
	}

	void Run()
	{
		while (running)
		{
			SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
			if (clientSocket != INVALID_SOCKET)
			{
				Client c{ clientSocket, clients.size() + 1, true };
				clients.push_back(c);
				if (clients.size() == 1)
				{
					Send(c, "WaitingForPlayer");
				}
				else if (clients.size() == 2)
				{
					std::thread(&Server::GameLoop, this, clients[0], clients[1]).detach();
				}
			}
		}
	}

	void Send(Client client, std::string msg)
	{
		send(client.socket, msg.c_str(), (int)msg.size(), 0);
	}

	std::string Receive(Client client)
	{
		char buffer[128] = {};
		int len = recv(client.socket, buffer, sizeof(buffer), 0);
		return (len > 0) ? std::string(buffer, len) : "";
	}

	void GameLoop(Client p1, Client p2)
	{
		char board[3][3];
		memset(board, ' ', sizeof(board));
		size_t turn = 1;
		int moves = 0;

		auto DrawBoard = [&]() -> std::string
			{
				std::string s;
				for (int i = 0; i < 3; ++i)
				{
					for (int j = 0; j < 3; ++j)
						s += board[i][j];
					s += '\n';
				}
				return s;
			};

		while (true)
		{
			Client& current = (turn == 1) ? p1 : p2;
			Client& other = (turn == 1) ? p2 : p1;
			Send(current, "YourMove\n" + DrawBoard());
			Send(other, "Waiting\n" + DrawBoard());
			std::string move = Receive(current);
			if (move.length() < 2) continue;
			int r = move[0] - '0';
			int c = move[1] - '0';
			if (r < 0 || r > 2 || c < 0 || c > 2 || board[r][c] != ' ') continue;
			board[r][c] = (turn == 1) ? 'X' : 'O';
			moves++;

			for (int i = 0; i < 3; i++)
			{
				if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ' ||
					board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
				{
					Send(current, "Win\n" + DrawBoard());
					Send(other, "Lose\n" + DrawBoard());
					return;
				}
			}
			if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ' ||
				board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
			{
				Send(current, "Win\n" + DrawBoard());
				Send(other, "Lose\n" + DrawBoard());
				return;
			}
			if (moves == 9)
			{
				Send(p1, "Draw\n" + DrawBoard());
				Send(p2, "Draw\n" + DrawBoard());
				return;
			}
			turn = (turn == 1) ? 2 : 1;
		}
	}
};

int main()
{
	Server::Settings s;
	s.port = "7777";
	s.debugMessages = true;
	Server server(s);
	while (true) Sleep(1000);
}