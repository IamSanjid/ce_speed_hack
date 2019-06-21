#pragma once

#include <Windows.h>
#include <iostream>
#include <winnt.h>
#include <fstream>

#include "Detours/detours.h"

#pragma comment(lib, "Detours/detours.lib")
#pragma comment(lib,"Kernel32.lib")
#pragma comment(lib,"Winmm.lib")

namespace Helper
{
	// custom helper function..
	void WriteTextToFile(std::string str)
	{
		std::ofstream myfile;
		myfile.open("OUTPUT_SPEEDHACK.log");
		myfile << (str + "\n").data();
		myfile.close();
	}
}

namespace Speedhack
{
	// native original functions
	extern"C" {
		static BOOL(WINAPI *originalQueryPerformanceCounter)(LARGE_INTEGER *performanceCounter) = QueryPerformanceCounter;
		static DWORD(WINAPI *originalGetTickCount)() = GetTickCount;
		static ULONGLONG(WINAPI *originalGetTickCount64)() = GetTickCount64;
		static DWORD(WINAPI *originalTimeGetTime)() = timeGetTime;
	}	

	// tried to follow CE :")
	class TSimpleLock
	{
	public:
		TSimpleLock()
		{
			owner = GetCurrentThreadId();
		}
		unsigned long count;
		DWORD owner;
	};

	// tried to follow CE :")
	TSimpleLock GTCLock;
	TSimpleLock QPCLock;


	void lock(TSimpleLock d)
	{
		auto tid = GetCurrentThreadId();
		if (d.owner == tid)
		{
			do {
				Sleep(0);
			} while (InterlockedExchange(&d.count, 1) == 0);
		}
		else {
			InterlockedIncrement(&d.count);
		}
	}


	void unlock(TSimpleLock d)
	{
		if (d.count == 1)
			d.owner = 0;
		InterlockedDecrement(&d.count);
		if (d.count < 0) Helper::WriteTextToFile("error -1, during unlocking: " + d.owner);
	}

	double multiplier = 1.0;																// Game speed multiplier

	LARGE_INTEGER initialtime64;
	LARGE_INTEGER initialoffset64;

	// QueryPerformanceCounter is generally what is used to calculate how much time has passed between frames. It will set the performanceCounter to the amount of micro seconds the machine has been running
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms644904(v=vs.85).aspx

	BOOL WINAPI newQueryPerformanceCounter(LARGE_INTEGER *counter) {
		lock(QPCLock);

		LARGE_INTEGER currentLi;
		LARGE_INTEGER falseLi;

		originalQueryPerformanceCounter(&currentLi);
		falseLi.QuadPart = static_cast<LONGLONG>(((currentLi.QuadPart - initialtime64.QuadPart) * multiplier) + initialoffset64.QuadPart);
		unlock(QPCLock);

		*counter = falseLi;
		return true;
	}


	DWORD initialtime;
	DWORD initialOffset;

	// GetTickCount can also be used to calculate time between frames, but is used less since it's less accurate than QueryPerformanceCounter
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms724408%28v=vs.85%29.aspx

	DWORD WINAPI newGetTickCount() {
		lock(GTCLock);

		auto currentTickCount = originalGetTickCount();
		auto falseTickCount = static_cast<DWORD>(((currentTickCount - initialtime) * multiplier) + initialOffset);

		unlock(GTCLock);
		return falseTickCount;																					// Return false tick count
	}

	ULONGLONG initialtime_tc64;
	ULONGLONG initialoffset_tc64;

	DWORD WINAPI newGetTickCount64() {
		lock(GTCLock);

		auto currentTickCount64 = originalGetTickCount64();
		auto falseTickCount64 = static_cast<DWORD>(((currentTickCount64 - initialtime_tc64) * multiplier) + initialoffset_tc64);

		unlock(GTCLock);
		return falseTickCount64;
	}

	DWORD prevTime;
	DWORD currentTime;
	DWORD falseTime;

	// timeGetTime can also be used to caluclate time between frames, as with GetTickCount it isn't as accurate as QueryPerformanceCounter
	// https://msdn.microsoft.com/en-us/library/windows/desktop/dd757629(v=vs.85).aspx

	DWORD WINAPI newTimeGetTime() {
		currentTime = originalTimeGetTime();
		falseTime += static_cast<DWORD>(((currentTime - prevTime) * multiplier));
		prevTime = currentTime;

		return falseTime;
	}

	//Called by createremotethread
	void InitializeSpeedHack(double speed) {
		lock(QPCLock);
		lock(GTCLock);

		originalQueryPerformanceCounter(&initialtime64);
		initialoffset64 = initialtime64;

		initialOffset = originalGetTickCount();
		initialtime = initialOffset;


		initialoffset_tc64 = originalGetTickCount64();
		initialtime_tc64 = initialoffset_tc64;

		prevTime = originalTimeGetTime();
		falseTime = prevTime;
		multiplier = speed;

		unlock(GTCLock);
		unlock(QPCLock);
	}

	// This should be called when the DLL is Injected. You should call this in a new Thread.
	void InintDLL(LPVOID hModule)
	{
		GTCLock = TSimpleLock();
		QPCLock = TSimpleLock();
		// Set initial values for hooked calculations
		originalQueryPerformanceCounter(&initialtime64);
		initialoffset64 = initialtime64;

		initialOffset = originalGetTickCount();
		initialtime = initialOffset;

		initialoffset_tc64 = originalGetTickCount64();
		initialtime_tc64 = initialoffset_tc64;

		prevTime = originalTimeGetTime();
		falseTime = prevTime;

		// ah detours; they are awesome!!
		DisableThreadLibraryCalls((HMODULE)hModule);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)originalQueryPerformanceCounter, newQueryPerformanceCounter);
		DetourAttach(&(PVOID&)originalGetTickCount, newGetTickCount);
		DetourAttach(&(PVOID&)originalGetTickCount64, newGetTickCount64);
		DetourAttach(&(PVOID&)originalTimeGetTime, newTimeGetTime);
		DetourTransactionCommit();
	}
}