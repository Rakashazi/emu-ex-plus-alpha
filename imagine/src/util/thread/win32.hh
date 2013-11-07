#pragma once
#include <engine-globals.h>
#include <logger/interface.h>
#include <util/DelegateFunc.hh>
#include <assert.h>
#include <util/windows/windows.h>

class ThreadWin32
{
public:
	bool running = 0;

	constexpr ThreadWin32() {}

	typedef DelegateFunc<ptrsize (ThreadWin32 &thread)> EntryDelegate;

	bool create(uint type, EntryDelegate entry)
	{
		this->entry = entry;

		hnd = CreateThread(nullptr, 0, wrapper, this, 0, nullptr);
		if(!hnd)
		{
			logErr("error in thread create");
			return 0;
		}
		logMsg("created wrapped thread %p", hnd);
		running = 1;
		return 1;
	}

	void join()
	{
		logMsg("joining thread %p", hnd);
		WaitForSingleObject(hnd, INFINITE);
	}

	EntryDelegate entry;
private:
	HANDLE hnd = nullptr;

private:
	static DWORD WINAPI wrapper(LPVOID lpParam)
	{
		auto &run = *((ThreadWin32*)lpParam);
		//logMsg("running thread func %p", run->entry);
		auto res = run.entry(run);
		run.running = 0;
		return 0;
	}
};
