#pragma once

#include <config.h>

#ifdef CONFIG_BASE_WIN32
#include "win32.hh"
#define ThreadSys ThreadWin32
#else
#include "pthread.hh"
#define ThreadSys ThreadPThread
#endif
