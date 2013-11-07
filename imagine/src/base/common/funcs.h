#pragma once

#include <mem/interface.h>

#ifdef CONFIG_FS
#include <fs/sys.hh>
#endif

#ifdef CONFIG_INPUT
#include <input/Input.hh>
#endif

#if defined CONFIG_BLUETOOTH
#include <bluetooth/sys.hh>
#endif

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/resource.h>
#endif

namespace Base
{

const char copyright[] = "Imagine is Copyright 2010-2013 Robert Broglia";

static void engineInit() ATTRS(cold);
static void engineInit()
{
	#if defined __unix__ || defined CONFIG_BASE_MACOSX
	struct rlimit stack;
	getrlimit(RLIMIT_STACK, &stack);
	stack.rlim_cur = 16 * 1024 * 1024;
	assert(stack.rlim_cur <= stack.rlim_max);
	setrlimit(RLIMIT_STACK, &stack);
		#ifndef NDEBUG
		getrlimit(RLIMIT_STACK, &stack);
		logMsg("stack limit %u:%u", (uint)stack.rlim_cur, (uint)stack.rlim_max);
		#endif
	#endif

	logDMsg("%s", copyright);
	logDMsg("compiled on %s %s", __DATE__, __TIME__);
	mem_init();
}

// needed by GCC when not compiling with libstdc++/libsupc++, or to override it
CLINK void __cxa_pure_virtual() { bug_exit("called pure virtual"); }

#if defined(__unix__) || defined(__APPLE__)
void sleepUs(int us)
{
	usleep(us);
}

void sleepMs(int ms)
{
	sleepUs(ms*1000);
}
#endif

static void processAppMsg(int type, int shortArg, int intArg, int intArg2)
{
	switch(type)
	{
		#if defined CONFIG_BLUETOOTH_BLUEZ || defined CONFIG_BLUETOOTH_ANDROID
		bcase MSG_BT_SCAN_STATUS_DELEGATE:
		{
			logMsg("got bluetooth adapter status delegate message");
			auto bta = BluetoothAdapter::defaultAdapter();
			bta->onScanStatus()(*bta, intArg, intArg2);
		}
		#endif
		bdefault:
		{
			if(type >= MSG_USER)
			{
				logMsg("got app message %d", type);
				Base::onAppMessage(type, shortArg, intArg, intArg2);
			}
		}
	}
}

}

#if defined CONFIG_BASE_X11 || defined CONFIG_BASE_ANDROID
static int getPollTimeout()
{
	// When waiting for events:
	// 1. If rendering, don't block
	// 2. Else block until next event
	int pollTimeout = Base::windowsArePosted() ? 0 :
		-1;
	/*if(pollTimeout == -1)
		logMsg("will poll for next event");*/
	return pollTimeout;
}
#endif

#if defined(__has_feature)
	#if __has_feature(address_sanitizer)
	#define CONFIG_BASE_NO_CUSTOM_NEW_DELETE
	#endif
#endif

#ifndef CONFIG_BASE_NO_CUSTOM_NEW_DELETE

void* operator new (std::size_t size)
#ifdef __EXCEPTIONS
	throw (std::bad_alloc)
#endif
{ return mem_alloc(size); }

void* operator new[] (std::size_t size)
#ifdef __EXCEPTIONS
	throw (std::bad_alloc)
#endif
{ return mem_alloc(size); }

#ifdef CONFIG_BASE_PS3
void *operator new(_CSTD size_t size, _CSTD size_t align)
	_THROW1(_XSTD bad_alloc)
{
	//logMsg("called aligned new, size %d @ %d byte boundary", (int)size, (int)align);
	return memalign(size, align);
}
#endif

void operator delete (void *o) noexcept { mem_free(o); }
void operator delete[] (void *o) noexcept { mem_free(o); }

#endif

#ifdef __EXCEPTIONS
namespace __gnu_cxx
{

EVISIBLE void __verbose_terminate_handler()
{
	logErr("terminated by uncaught exception");
  abort();
}

}
#endif
