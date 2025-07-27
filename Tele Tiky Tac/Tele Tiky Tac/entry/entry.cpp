#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
//#define NONLS             // All NLS defines and routines
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions
#include "raylib.h"
#include "network/network.h"

void DrawCenteredText(std::string text, int fontSize, Color color = BLACK)
{
	int textWidth = MeasureText(text.c_str(), fontSize);
	int screenWidth = GetScreenWidth();
	int screenHeight = GetScreenHeight();

	int x = (screenWidth - textWidth) / 2;
	int y = (screenHeight - fontSize) / 2;

	DrawText(text.c_str(), x, y, fontSize, BLACK);
}

std::string queryReceive;
void ReceiveMsgThread(Client& client)
{
	std::thread([&client]()
		{
			while (true)
			{
				queryReceive = client.Receive();
			}
		}
	).detach();
}

int main()
{
	// Connecting to the sever
	Client::Settings settings;
	settings.port = "3709";
	settings.ip = "127.0.0.1";
	settings.debugMessages = false;
	settings.alwaysConnect = true;
	settings.secondsTimeout = -1;
	Client client(settings);
	ReceiveMsgThread(client);

	// Setting raylib
	InitWindow(900, 700, "Tele Tiky Tac");
	SetTargetFPS(60);

	// Game loop
	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(RAYWHITE);
		if (!client.IsConnected())
		{
			DrawCenteredText("Server isn't running", 32);
		}
		else
		{
			if (queryReceive.find("WaitScreen") != std::string::npos) // idk why yet
			{
				DrawCenteredText("Waiting for second player", 32);
			}
			else if (queryReceive.starts_with("StartIn"))
			{
				DrawCenteredText("Starting in " + queryReceive.substr(7), 32);
			}
			else if (queryReceive.find("Start") != std::string::npos)
			{
				while (true)
				{
					if (queryReceive.find("GameEnd") != std::string::npos) // idk why yet
					{
						DrawCenteredText("Game Ended", 32);
						break;
					}
				}
			}
		}

		EndDrawing();
	}
	CloseWindow();
}