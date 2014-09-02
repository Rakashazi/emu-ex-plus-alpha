#pragma once

#ifdef __PPU__
#include "ps3.hh"
using TimeSys = TimePs3;
#elif defined __APPLE__
#include "TimeMach.hh"
using TimeSys = TimeMach;
#elif defined _WIN32
#include "TimeWin32.hh"
using TimeSys = TimeWin32;
#else
#include "TimeTimespec.hh"
using TimeSys = TimeTimespec;
#endif

namespace IG
{

template <class FUNC>
static TimeSys timeFunc(FUNC func)
{
	auto before = TimeSys::now();
	func();
	auto after = TimeSys::now();
	return after - before;
}

template <class FUNC>
static TimeSys timeFuncDebug(FUNC func)
{
	#ifdef NDEBUG
	// execute directly without timing
	func();
	return {};
	#else
	return timeFunc(func);
	#endif
}

}
