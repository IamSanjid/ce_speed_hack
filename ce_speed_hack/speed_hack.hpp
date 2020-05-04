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

	void lock(TSimpleLock& d)
	{
		auto tid = GetCurrentThreadId();
		if (d.owner != tid)
		{
			do {
				Sleep(0);
			} while (InterlockedExchange(&d.count, 1) == 0);
			d.owner = tid;
		}
		else {
			InterlockedIncrement(&d.count);
		}
	}

	void unlock(TSimpleLock& d)
	{
		if (d.count == 1)
			d.owner = 0;
		InterlockedDecrement(&d.count);
		if (d.count < 0) Helper::WriteTextToFile("error -1, during unlocking: " + d.owner);
	}

	TSimpleLock GTCLock;
	TSimpleLock QPCLock;

	template<class T>
	class SpeedHackClass
	{
	private:
		double speed = 0;
		T initialoffset;
		T initialtime;
	public:
		SpeedHackClass()
		{
			speed = 1.0;
		}
		SpeedHackClass(T _initialtime, T _initialoffset, double _speed = 1.0)
		{
			speed = _speed;
			initialoffset = _initialoffset;
			initialtime = _initialtime;
		}

		double get_speed() const { return speed; }

		T get(T currentTime)
		{	
			T false_val = (T)((currentTime - initialtime) * speed) + initialoffset;
			return (T)false_val;
		}

		void set_speed(double _speed)
		{
			speed = _speed;
		}
	};


	SpeedHackClass<LONGLONG> h_QueryPerformanceCounter;
	SpeedHackClass<DWORD> h_GetTickCount;
	SpeedHackClass<ULONGLONG> h_GetTickCount64;
	SpeedHackClass<DWORD> h_GetTime;

	double lastspeed = 1.0; // Game speed lastspeed

	// QueryPerformanceCounter is generally what is used to calculate how much time has passed between frames. It will set the performanceCounter to the amount of micro seconds the machine has been running
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms644904(v=vs.85).aspx

	BOOL WINAPI newQueryPerformanceCounter(LARGE_INTEGER *counter) {
		lock(QPCLock);
		LARGE_INTEGER currentLi;
		LARGE_INTEGER falseLi;
		originalQueryPerformanceCounter(&currentLi);
		falseLi.QuadPart = h_QueryPerformanceCounter.get(currentLi.QuadPart);
		unlock(QPCLock);

		*counter = falseLi;
		return true;
	}

	// GetTickCount can also be used to calculate time between frames, but is used less since it's less accurate than QueryPerformanceCounter
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms724408%28v=vs.85%29.aspx

	DWORD WINAPI newGetTickCount() {
		lock(GTCLock);
		auto res = h_GetTickCount.get(originalGetTickCount());
		unlock(GTCLock);

		return res;																					// Return false tick count
	}

	// GetTickCount64 can also be used to calculate time between frames, but is used less since it's less accurate than QueryPerformanceCounter
	//https://docs.microsoft.com/en-us/windows/desktop/api/sysinfoapi/nf-sysinfoapi-gettickcount64

	ULONGLONG WINAPI newGetTickCount64() {
		lock(GTCLock);
		auto res = h_GetTickCount64.get(originalGetTickCount64());
		unlock(GTCLock);

		return res;
	}

	// timeGetTime can also be used to caluclate time between frames, as with GetTickCount it isn't as accurate as QueryPerformanceCounter
	// https://msdn.microsoft.com/en-us/library/windows/desktop/dd757629(v=vs.85).aspx

	DWORD WINAPI newTimeGetTime() {
		return h_GetTime.get(originalTimeGetTime());
	}

	LARGE_INTEGER initialtime64;
	LARGE_INTEGER initialoffset64;

	//Called by createremotethread
	void InitializeSpeedHack(double speed) {
		lock(QPCLock);
		lock(GTCLock);

		originalQueryPerformanceCounter(&initialtime64);
		newQueryPerformanceCounter(&initialoffset64);
		/*
		initialOffset = newGetTickCount();
		initialtime = originalGetTickCount();


		initialoffset_tc64 = newGetTickCount64();
		initialtime_tc64 = originalGetTickCount64();

		prevTime = originalTimeGetTime();
		falseTime = prevTime;*/

		h_QueryPerformanceCounter = SpeedHackClass<LONGLONG>(initialtime64.QuadPart, initialoffset64.QuadPart, speed);
		h_GetTickCount = SpeedHackClass<DWORD>(originalGetTickCount(), newGetTickCount(), speed);
		h_GetTickCount64 = SpeedHackClass<ULONGLONG>(originalGetTickCount64(), newGetTickCount64(), speed);
		h_GetTime = SpeedHackClass<DWORD>(originalTimeGetTime(), newTimeGetTime(), speed);

		lastspeed = speed;

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

		h_QueryPerformanceCounter = SpeedHackClass<LONGLONG>(initialtime64.QuadPart, initialoffset64.QuadPart);
		h_GetTickCount = SpeedHackClass<DWORD>(originalGetTickCount(), originalGetTickCount());
		h_GetTickCount64 = SpeedHackClass<ULONGLONG>(originalGetTickCount64(), originalGetTickCount64());
		h_GetTime = SpeedHackClass<DWORD>(originalTimeGetTime(), originalTimeGetTime());

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
