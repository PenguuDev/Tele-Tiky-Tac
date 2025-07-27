#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include "raylib.h"
#include "network/network.h"
#include <thread>
#include <string>

SOCKET ConnectToServer(const char* ip, const char* port)
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	addrinfo hints{}, * res;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(ip, port, &hints, &res);
	SOCKET s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (connect(s, res->ai_addr, (int)res->ai_addrlen) != 0)
	{
		closesocket(s);
		s = INVALID_SOCKET;
	}
	freeaddrinfo(res);
	return s;
}

std::string latestMsg = "";
void ReceiveThread(SOCKET sock)
{
	char buffer[512];
	while (true)
	{
		int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (len <= 0) break;
		buffer[len] = 0;
		latestMsg = buffer;
	}
}

int main()
{
	InitWindow(600, 200, "Connect to Server");
	SetTargetFPS(60);

	char ip[64] = "127.0.0.1";
	char port[16] = "7777";
	int ipLen = (int)strlen(ip);
	int portLen = (int)strlen(port);
	bool editingIp = true;
	bool connectRequested = false;
	bool connectFailed = false;
	SOCKET s = INVALID_SOCKET;

	while (!WindowShouldClose() && !connectRequested)
	{
		BeginDrawing();
		ClearBackground(RAYWHITE);

		DrawText("Enter IP:", 50, 40, 20, DARKGRAY);
		DrawRectangle(150, 35, 300, 30, LIGHTGRAY);
		DrawText(ip, 155, 40, 20, BLACK);

		DrawText("Enter Port:", 50, 90, 20, DARKGRAY);
		DrawRectangle(150, 85, 300, 30, LIGHTGRAY);
		DrawText(port, 155, 90, 20, BLACK);

		DrawRectangle(470, 35, 80, 80, SKYBLUE);
		DrawText("Connect", 485, 70, 20, BLACK);

		if (connectFailed)
		{
			DrawText("Failed to connect!", 150, 130, 20, RED);
		}

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		{
			Vector2 mouse = GetMousePosition();
			if (mouse.x >= 470 && mouse.x <= 550 && mouse.y >= 35 && mouse.y <= 115)
			{
				s = ConnectToServer(ip, port);
				if (s != INVALID_SOCKET)
				{
					connectRequested = true;
				}
				else
				{
					connectFailed = true;
				}
			}
			else if (mouse.y >= 35 && mouse.y <= 65)
			{
				editingIp = true;
			}
			else if (mouse.y >= 85 && mouse.y <= 115)
			{
				editingIp = false;
			}
		}

		int key = GetCharPressed();
		while (key > 0)
		{
			if (key >= 32 && key <= 126)
			{
				if (editingIp && ipLen < 63)
				{
					ip[ipLen++] = (char)key;
					ip[ipLen] = 0;
				}
				else if (!editingIp && portLen < 15)
				{
					port[portLen++] = (char)key;
					port[portLen] = 0;
				}
			}
			key = GetCharPressed();
		}

		if (IsKeyPressed(KEY_BACKSPACE))
		{
			if (editingIp && ipLen > 0)
				ip[--ipLen] = 0;
			else if (!editingIp && portLen > 0)
				port[--portLen] = 0;
		}

		EndDrawing();
	}

	if (!connectRequested)
	{
		CloseWindow();
		return 1;
	}

	CloseWindow();

	InitWindow(600, 600, "Tic Tac Toe");
	SetTargetFPS(60);

	std::thread(ReceiveThread, s).detach();

	char board[3][3] = {};

	while (!WindowShouldClose())
	{
		BeginDrawing();

		if (latestMsg.find("YourMove") != std::string::npos)
			ClearBackground(Color{ 200, 255, 200, 255 }); // light green
		else if (latestMsg.find("Waiting") != std::string::npos)
			ClearBackground(Color{ 255, 200, 200, 255 }); // light red
		else
			ClearBackground(RAYWHITE);

		if (latestMsg.find("WaitingForPlayer") != std::string::npos)
		{
			Vector2 sz = MeasureTextEx(GetFontDefault(), "Waiting for player...", 30, 1);
			DrawText("Waiting for player...", (int)((600 - sz.x) / 2), (int)((600 - sz.y) / 2), 30, DARKGRAY);
		}
		else if (latestMsg.find("Win") != std::string::npos)
		{
			Vector2 sz = MeasureTextEx(GetFontDefault(), "You Win!", 40, 1);
			DrawText("You Win!", (int)((600 - sz.x) / 2), (int)((600 - sz.y) / 2), 40, GREEN);
		}
		else if (latestMsg.find("Lose") != std::string::npos)
		{
			Vector2 sz = MeasureTextEx(GetFontDefault(), "You Lose!", 40, 1);
			DrawText("You Lose!", (int)((600 - sz.x) / 2), (int)((600 - sz.y) / 2), 40, RED);
		}
		else if (latestMsg.find("Draw") != std::string::npos)
		{
			Vector2 sz = MeasureTextEx(GetFontDefault(), "Draw!", 40, 1);
			DrawText("Draw!", (int)((600 - sz.x) / 2), (int)((600 - sz.y) / 2), 40, GRAY);
		}
		else
		{
			DrawLine(200, 0, 200, 600, BLACK);
			DrawLine(400, 0, 400, 600, BLACK);
			DrawLine(0, 200, 600, 200, BLACK);
			DrawLine(0, 400, 600, 400, BLACK);

			if (latestMsg.find("YourMove") != std::string::npos || latestMsg.find("Waiting") != std::string::npos)
			{
				std::string boardData = latestMsg.substr(latestMsg.find("\n") + 1);
				for (int i = 0, k = 0; i < 3; i++)
					for (int j = 0; j < 3; j++, k++)
						board[i][j] = boardData[k + i];

				for (int i = 0; i < 3; i++)
					for (int j = 0; j < 3; j++)
					{
						if (board[i][j] != ' ')
						{
							char symbol[2] = { board[i][j], '\0' };
							int fontSize = 80;
							Vector2 textSize = MeasureTextEx(GetFontDefault(), symbol, (float)fontSize, 1);
							float posX = j * 200 + (200 - textSize.x) / 2;
							float posY = i * 200 + (200 - textSize.y) / 2;
							DrawText(symbol, (int)posX, (int)posY, fontSize, BLUE);
						}
					}

				if (latestMsg.find("YourMove") != std::string::npos && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
				{
					Vector2 mouse = GetMousePosition();
					int row = mouse.y / 200;
					int col = mouse.x / 200;
					std::string move = std::to_string(row) + std::to_string(col);
					send(s, move.c_str(), (int)move.size(), 0);
				}
			}
		}

		EndDrawing();
	}

	closesocket(s);
	WSACleanup();
	CloseWindow();
	return 0;
}
