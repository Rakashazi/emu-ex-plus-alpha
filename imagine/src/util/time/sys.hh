#pragma once

#include <config.h>

#ifdef CONFIG_BASE_PS3
	#include "ps3.hh"
	#define TimeSys TimePs3
#else
	#include "timeval.hh"
	#define TimeSys TimeTimeval
#endif
