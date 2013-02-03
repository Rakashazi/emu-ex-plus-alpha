#pragma once

#include <config.h>

#ifdef CONFIG_BASE_PS3
	#include "ps3.hh"
	#define TimeSys TimePs3
#elif defined __APPLE__
	#include "TimeMach.hh"
	#define TimeSys TimeMach
#else
	#include "TimeTimespec.hh"
	#define TimeSys TimeTimespec
#endif
