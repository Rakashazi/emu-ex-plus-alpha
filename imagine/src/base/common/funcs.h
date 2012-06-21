#pragma once

#include <mem/interface.h>

#ifdef CONFIG_FS
	#include <fs/Fs.hh>
#endif

#ifdef CONFIG_INPUT
	#include <input/interface.h>
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

#ifdef CONFIG_BLUEZ
	#include <bluetooth/sys.hh>
#endif

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace Base
{

#if defined(CONFIG_BASE_PS3)
	uint refreshRate() { return 60; } // hard-code for now
#else
	static uint refreshRate_ = 0;
	uint refreshRate() { return refreshRate_; }
#endif

static fbool triggerGfxResize = 0;
static uint newXSize = 0, newYSize = 0;

fbool gfxUpdate = 0;
static void generic_displayNeedsUpdate()
{
	//logMsg("posting display update");
	gfxUpdate = 1;
}

static Window mainWin;
const Window &window()
{
	return mainWin;
}

#ifdef CONFIG_GFX
static int generic_resizeEvent(uint x, uint y, bool force = 0)
{
	newXSize = x; newYSize = y;
	uint oldX = Gfx::viewPixelWidth(), oldY = Gfx::viewPixelHeight();
	// do gfx_resizeDisplay only if the window-size changed
	if(force || (newXSize != Gfx::viewPixelWidth()) || (newYSize != Gfx::viewPixelHeight()))
	{
		logMsg("resizing display area %dx%d -> %dx%d", oldX, oldY, x, y);
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
	logDMsg("%s", copyright);
	logDMsg("compiled on %s %s", __DATE__, __TIME__);
	mem_init();
	
	#ifdef CONFIG_GFX
		doOrExit(Gfx::init());
		doOrExit(Gfx::setOutputVideoMode(newXSize, newYSize));
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
		Gfx::resizeDisplay(newXSize, newYSize);
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

#ifndef CONFIG_SUPCXX
	// needed by GCC when not compiling with libstdc++/libsupc++
	CLINK void __cxa_pure_virtual() { bug_exit("called pure virtual"); }
#endif

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
		#ifdef CONFIG_BLUEZ
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
			BluetoothAdapterSys::defaultAdapter()->statusDelegate().invoke(intArg, intArg2);
		}
		#endif
		#if CONFIG_ENV_WEBOS_OS >= 3
		bcase MSG_ORIENTATION_CHANGE:
		{
			logMsg("got orientation change message");
			uint o = shortArg;
			if(o != Gfx::VIEW_ROTATE_180)
			{
				logMsg("new orientation %s", Gfx::orientationName(o));
				Gfx::preferedOrientation = o;
				Gfx::setOrientation(Gfx::preferedOrientation);
			}
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

void* operator new (size_t size)
#ifdef __EXCEPTIONS
	throw ()
#endif
{ return mem_alloc(size); }

void* operator new[] (size_t size)
#ifdef __EXCEPTIONS
	throw ()
#endif
{ return mem_alloc(size); }

void *operator new (size_t size, void *o)
#ifdef __EXCEPTIONS
	throw ()
#endif
{
	//logMsg("called placement new, %d bytes @ %p", (int)size, o);
	return o;
}

//void* operator new (size_t size, long unsigned int) { return mem_alloc(size); }

void operator delete (void *o) { mem_free(o); }
void operator delete[] (void *o) { mem_free(o); }
