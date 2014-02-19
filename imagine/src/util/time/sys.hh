#pragma once

#include <config.h>

#ifdef __PPU__
#include "ps3.hh"
using TimeSys = TimePs3;
#elif defined __APPLE__
#include "TimeMach.hh"
using TimeSys = TimeMach;
#elif defined CONFIG_BASE_WIN32
#include "TimeWin32.hh"
using TimeSys = TimeWin32;
#else
#include "TimeTimespec.hh"
using TimeSys = TimeTimespec;
#endif
