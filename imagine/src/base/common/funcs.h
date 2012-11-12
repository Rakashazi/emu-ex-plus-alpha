#pragma once

#include <mem/interface.h>

#ifdef CONFIG_FS
	#include <fs/Fs.hh>
#endif

#ifdef CONFIG_INPUT
	#include <input/Input.hh>
#endif

#ifdef CONFIG_GFX
	#include <gfx/Gfx.hh>
#endif

#ifdef CONFIG_RESOURCE
	#include <resource2/Resource.h>
#endif

#ifdef CONFIG_AUDIO
	#include <audio/Audio.hh>
#endif

#if defined CONFIG_BLUEZ || defined CONFIG_ANDROIDBT
	#include <bluetooth/sys.hh>
#endif

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>

#include <sys/resource.h>
#endif

namespace Base
{

#if defined(CONFIG_BASE_PS3)
	uint refreshRate() { return 60; } // hard-code for now
#else
	static uint refreshRate_ = 0;
	uint refreshRate() { return refreshRate_; }
#endif

static bool triggerGfxResize = 0;
static Window mainWin, currWin;
bool gfxUpdate = 0;
static void generic_displayNeedsUpdate()
{
	//logMsg("posting display update");
	gfxUpdate = 1;
}

const Window &window()
{
	return mainWin;
}

#ifdef CONFIG_GFX
static int generic_resizeEvent(const Window &win, bool force = 0)
{
	// do gfx_resizeDisplay only if the window-size changed
	if(force || currWin != win)
	{
		logMsg("resizing display area %d:%d:%d:%d -> %d:%d:%d:%d",
				currWin.rect.x, currWin.rect.y, currWin.rect.x2, currWin.rect.y2,
				win.rect.x, win.rect.y, win.rect.x2, win.rect.y2);
		currWin = win;
		triggerGfxResize = 1;
		gfxUpdate = 1;
		return 1;
	}
	return 0;
}
#endif

const char copyright[] = "Imagine is Copyright 2010, 2011 Robert Broglia";

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
	
	#ifdef CONFIG_GFX
		doOrExit(Gfx::init());
		currWin = mainWin;
		doOrExit(Gfx::setOutputVideoMode(mainWin));
	#endif

	#ifdef CONFIG_INPUT
		doOrExit(Input::init());
	#endif	
		
	#ifdef CONFIG_AUDIO
		doOrExit(Audio::init());
	#endif

	doOrExit(onInit());
}

static uint runEngine()
{
	#ifdef CONFIG_GFX
	if(unlikely(triggerGfxResize))
	{
		Gfx::resizeDisplay(currWin);
		triggerGfxResize = 0;
	}
	#endif

	int frameRendered = 0;
	#ifdef CONFIG_GFX
		if(likely(gfxUpdate))
		{
			gfxUpdate = 0;
			Gfx::renderFrame();
			frameRendered = 1;
			//logMsg("rendered frame");
		}
		else
		{
			//logDMsg("skipped render");
		}
	#endif

	return frameRendered;
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
		#if defined CONFIG_BLUEZ || defined CONFIG_ANDROIDBT
		/*case MSG_INPUT:
		{
			Input::onInputEvent(InputEvent(shortArg, intArg & 0xFFFF, intArg2, intArg >> 16));
		}
		break;
		case MSG_INPUTDEV_CHANGE:
		{
			logMsg("got input dev change message");
			onInputDevChange((InputDevChange){ (uint)intArg2, (uint)intArg, (uint)shortArg });
		}
		break;*/
		/*case MSG_BT:
		{
			logMsg("got bluetooth connect message");
			//Bluetooth::connectFunc(intArg);
		}
		break;*/
		bcase MSG_BT_SCAN_STATUS_DELEGATE:
		{
			logMsg("got bluetooth adapter status delegate message");
			BluetoothAdapter::defaultAdapter()->statusDelegate().invoke(intArg, intArg2);
		}
		#if defined CONFIG_ANDROIDBT
		bcase MSG_BT_SOCKET_STATUS_DELEGATE:
		{
			logMsg("got bluetooth socket status delegate message");
			auto s = (BluetoothSocket*)intArg2;
			s->onStatusDelegate().invoke(*s, intArg);
		}
		#endif
		#endif
		#if CONFIG_ENV_WEBOS_OS >= 3
		bcase MSG_ORIENTATION_CHANGE:
		{
			logMsg("got orientation change message");
			uint o = shortArg;
			logMsg("new orientation %s", Gfx::orientationName(o));
			Gfx::preferedOrientation = o;
			Gfx::setOrientation(Gfx::preferedOrientation);
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

#ifdef USES_POLL_WAIT_TIMER

#include <base/common/PollWaitTimer.hh>

DLList<PollWaitTimer>::Node DLListNodeArray(PollWaitTimer::timerListNode, 4);
DLList<PollWaitTimer> PollWaitTimer::timerList {timerListNode};

namespace Base
{

void cancelCallback(CallbackRef *ref)
{
	if(ref)
		((PollWaitTimer*)ref)->remove();
}

CallbackRef *callbackAfterDelay(CallbackDelegate callback, int ms)
{
	PollWaitTimer timer(callback);
	if(!timer.add(ms))
	{
		return nullptr;
	}
	return (CallbackRef*)timer.timerList.first();
}

}

#endif

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

/*void *operator new (size_t size, void *o)
#ifdef __EXCEPTIONS
	throw ()
#endif
{
	//logMsg("called placement new, %d bytes @ %p", (int)size, o);
	return o;
}*/

//void* operator new (size_t size, long unsigned int) { return mem_alloc(size); }

void operator delete (void *o) { mem_free(o); }
void operator delete[] (void *o) { mem_free(o); }

#ifdef __EXCEPTIONS
namespace __gnu_cxx
{

EVISIBLE void __verbose_terminate_handler()
{
	logMsg("terminated by uncaught exception");
  abort();
}

}
#endif
