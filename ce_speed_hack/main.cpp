#include "speed_hack.hpp"

void MAIN(LPVOID hModule)
{
	bool ac = false;
	auto lastSpeed = 1.0;
	while (true)
	{
		if (GetKeyState(VK_SHIFT) & 0x8000)
		{
			if (!ac)
			{
				AllocConsole();
				freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
#ifdef IS_DEBUG
				std::cout << "DEBUG: YES" << std::endl;
#else
				std::cout << "DEBUG: NO" << std::endl;
#endif
				ac = true;
			}
			if (lastSpeed != 5.0)
			{
				Speedhack::InitializeSpeedHack(5.0);
				lastSpeed = 5.0;
				std::cout << "SPEED: " << Speedhack::multiplier << std::endl;
			}
		}
		else
		{
			if (lastSpeed != 1.0)
			{
				Speedhack::InitializeSpeedHack(1.0);
				lastSpeed = 1.0;
				std::cout << "SPEED: " << Speedhack::multiplier << std::endl;
			}
		}
		Sleep(1);
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