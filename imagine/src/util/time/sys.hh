#pragma once

#include <config.h>

#ifdef __PPU__
	#include "ps3.hh"
	#define TimeSys TimePs3
#elif defined __APPLE__
	#include "TimeMach.hh"
	#define TimeSys TimeMach
#elif defined CONFIG_BASE_WIN32
	#include "TimeWin32.hh"
	#define TimeSys TimeWin32
#else
	#include "TimeTimespec.hh"
	#define TimeSys TimeTimespec
#endif
