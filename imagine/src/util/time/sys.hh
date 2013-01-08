#pragma once

#include <config.h>

#ifdef CONFIG_BASE_PS3
	#include "ps3.hh"
	#define TimeSys TimePs3
#elif defined CONFIG_BASE_IOS
	// TODO: use Mach time
	#include "TimeTimeval.hh"
	#define TimeSys TimeTimeval
#else
	#include "TimeTimespec.hh"
	#define TimeSys TimeTimespec
#endif
