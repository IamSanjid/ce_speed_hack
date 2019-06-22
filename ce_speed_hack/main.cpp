#include "speed_hack.hpp"

void clear() {
	COORD topLeft = { 0, 0 };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	SetConsoleCursorPosition(console, topLeft);
}

void MAIN(LPVOID hModule)
{
	bool ac = false;	
	while (true)
	{
		if (GetKeyState(VK_SHIFT) & 0x8000)
		{
			if (!ac)
			{
				AllocConsole();				
				freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
				clear();
#ifdef IS_DEBUG
				std::cout << "DEBUG: YES" << std::endl;
#else
				std::cout << "DEBUG: NO" << std::endl;
#endif
				ac = true;
			}
			if (Speedhack::lastspeed != 2.0)
			{
				Speedhack::InitializeSpeedHack(2.0);
				clear();
#ifdef IS_DEBUG
				std::cout << "DEBUG: YES" << std::endl;
#else
				std::cout << "DEBUG: NO" << std::endl;
#endif
				std::cout << "SPEED: " << Speedhack::lastspeed << std::endl;
			}
		}
		else
		{
			if (Speedhack::lastspeed != 1.0)
			{
				Speedhack::InitializeSpeedHack(1.0);
				clear();
#ifdef IS_DEBUG
				std::cout << "DEBUG: YES" << std::endl;
#else
				std::cout << "DEBUG: NO" << std::endl;
#endif
				std::cout << "SPEED: " << Speedhack::lastspeed << std::endl;
			}
		}
		Sleep(100);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{

	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Speedhack::InintDLL, (LPVOID)hModule, 0, NULL);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MAIN, (LPVOID)hModule, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;

	}
	return TRUE;
}