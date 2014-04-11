#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/engine-globals.h>
#include <imagine/logger/interface.h>
#include <imagine/util/DelegateFunc.hh>
#include <assert.h>
#include <imagine/util/windows/windows.h>

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
