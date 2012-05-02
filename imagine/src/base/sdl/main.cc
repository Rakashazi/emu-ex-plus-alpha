/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "base:sdl"
#include <engine-globals.h>

#include <unistd.h>
#include <SDL/SDL.h>
#ifdef CONFIG_BASE_SDL_PDL
	#include <PDL.h>
	#include <GLES/gl.h>
#endif

#include <gfx/Gfx.hh>
#include <input/interface.h>
#include <logger/interface.h>

#include <base/Base.hh>
#include <base/common/funcs.h>

#include "input.hh"

namespace Base
{

const char *appPath = 0;
uint appState = APP_RUNNING;

static SDL_Surface* drawContext;

CallResult openGLInit()
{
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	/*SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 2);*/
	return OK;
}

CallResult openGLSetOutputVideoMode(uint x, uint y)
{
	#ifdef CONFIG_ENV_WEBOS
	drawContext = SDL_SetVideoMode(0, 0, 0, SDL_OPENGL); // always full-screen
	#else
	drawContext = SDL_SetVideoMode(x, y, 0, SDL_OPENGL /*| SDL_RESIZABLE*/);
	#endif
	if(!drawContext)
	{
		logErr("error setting video mode");
		return INVALID_PARAMETER;
	}
	return OK;
}

CallResult openGLSetMultisampleVideoMode(uint x, uint y)
{
	/*SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	drawContext = SDL_SetVideoMode(x, y, 0, SDL_OPENGL /*| SDL_RESIZABLE*//*);
	if(drawContext == NULL)
	{
		logErr("error setting multisample video mode");
		return INVALID_PARAMETER;
	}*/
	return OK;
}

void openGLUpdateScreen()
{
	SDL_GL_SwapBuffers();
}

void exitVal(int returnVal)
{
	appState = APP_EXITING;
	onExit(0);
	#ifdef CONFIG_BASE_SDL_PDL
		PDL_Quit();
	#endif
	SDL_Quit();
	::exit(returnVal);
}
void abort() { ::abort(); }

static int globalArgc;
static char** globalArgv;
uint numArgs() { return(globalArgc); }
char * getArg(uint arg) { return(globalArgv[arg]); }

void displayNeedsUpdate() { generic_displayNeedsUpdate(); }

#ifdef CONFIG_ENV_WEBOS
void statusBarOrientation(uint o)
{
	switch(o)
	{
		case Gfx::VIEW_ROTATE_0: PDL_SetOrientation(PDL_ORIENTATION_0);
		break;
		case Gfx::VIEW_ROTATE_270: PDL_SetOrientation(PDL_ORIENTATION_90);
		break;
		case Gfx::VIEW_ROTATE_90: PDL_SetOrientation(PDL_ORIENTATION_270);
		break;
		case Gfx::VIEW_ROTATE_180: PDL_SetOrientation(PDL_ORIENTATION_180);
		break;
	}
}

static void setupScreenSizeFromDevice(PDL_ScreenMetrics &m)
{
	Gfx::viewMMWidth_ = ((float)m.horizontalPixels / (float)m.horizontalDPI) * 25.4;
	Gfx::viewMMHeight_ = ((float)m.verticalPixels / (float)m.verticalDPI) * 25.4;
	if(m.aspectRatio > (double)1.0) // Pre's pixels aren't square, we don't directly support this yet so scale the DPI
	{
		logMsg("screen has non-square pixels");
		Gfx::viewMMWidth_ *= m.aspectRatio;
	}
}
#endif

static TimerCallbackFunc timerCallbackFunc = 0;
static void *timerCallbackFuncCtx = 0;
static SDL_TimerID sdlTimerId = 0;
static Uint32 sdlTimerCallback(Uint32 interval, void* param)
{
	logMsg("sending SDL_USEREVENT for timer");
	SDL_Event ev = { SDL_USEREVENT };
	ev.user.code = 0;
	ev.user.data1 = (void*)timerCallbackFunc;
	ev.user.data2 = 0;
	SDL_PushEvent(&ev);
	sdlTimerId = 0;
	return 0;
}

void sendMessageToMain(ThreadPThread &, int type, int shortArg, int intArg, int intArg2)
{
	logMsg("sending SDL_USEREVENT for thread message");
	SDL_Event ev = { SDL_USEREVENT };
	ev.user.code = (shortArg & 0xFFFF) | (type << 16);
	ev.user.data1 = (void*)intArg;
	ev.user.data2 = (void*)intArg2;
}

void setTimerCallback(TimerCallbackFunc f, void *ctx, int ms)
{
	if(sdlTimerId)
	{
		SDL_RemoveTimer(sdlTimerId);
	}
	if(!f)
	{
		logMsg("canceling callback");
		timerCallbackFunc = 0;
		return;
	}
	logMsg("setting callback to run in %d ms", ms);
	timerCallbackFunc = f;
	timerCallbackFuncCtx = ctx;
	sdlTimerId = SDL_AddTimer(ms, sdlTimerCallback, 0);
}

static void eventHandler(SDL_Event &event)
{
	/*Uint8 *keys;
	keys = SDL_GetKeyState(NULL);
	if ( keys[SDLK_ESCAPE] == SDL_PRESSED ) {
	exit(0);*/

	switch(event.type)
	{
		bcase SDL_USEREVENT:
		{
			logMsg("got SDL_USEREVENT");
			uint id = event.user.code >> 16;
			if(id == 0)
			{
				if(timerCallbackFunc && event.user.data1 == timerCallbackFunc)
				{
					logMsg("running callback");
					timerCallbackFunc(timerCallbackFuncCtx);
					timerCallbackFunc = 0;
					if(appState != APP_RUNNING)
						gfxUpdate = 0; // cancel gfx update if app not active
				}
			}
			else
			{
				processAppMsg(id >> 16, event.user.code & 0xFFFF, (int)event.user.data1, (int)event.user.data1);
			}
		}

		bcase SDL_ACTIVEEVENT:
		{
			logMsg("got SDL_ACTIVEEVENT");
			if(event.active.state & SDL_APPACTIVE)
			{
				appState = event.active.gain ? APP_RUNNING : APP_PAUSED;
				onFocusChange(event.active.gain);
				#if defined(CONFIG_ENV_WEBOS) // redraw after being un-carded
					if(appState == APP_RUNNING)
						gfxUpdate = 1;
				#endif
			}
		}
		
		/*bcase SDL_VIDEORESIZE:
		{
			// not supported due to SDL limitations
		}*/
		
		bcase SDL_VIDEOEXPOSE:
		{
			logMsg("got SDL_VIDEOEXPOSE");
			#ifdef CONFIG_ENV_WEBOS
				// needed to make sure buffer is properly cleared at app start
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			#endif
			gfxUpdate = 1;
		}

		bcase SDL_KEYDOWN:
		{
			Input::keyEvent(event.key.keysym.sym, INPUT_PUSHED);
		}

		bcase SDL_KEYUP:
		{
			Input::keyEvent(event.key.keysym.sym, INPUT_RELEASED);
		}

		bcase SDL_MOUSEMOTION:
		{
			int p = 0;
			#ifdef CONFIG_BASE_SDL_PDL
			p = event.motion.which;
			#endif
			Input::mouseEvent(0, p, INPUT_MOVED, event.motion.x, event.motion.y);
		}

		bcase SDL_MOUSEBUTTONDOWN:
		{
			int p = 0;
			#ifdef CONFIG_BASE_SDL_PDL
			p = event.button.which;
			#endif
			Input::mouseEvent(event.button.button, p, INPUT_PUSHED, event.button.x, event.button.y);
		}

		bcase SDL_MOUSEBUTTONUP:
		{
			int p = 0;
			#ifdef CONFIG_BASE_SDL_PDL
			p = event.button.which;
			#endif
			Input::mouseEvent(event.button.button, p, INPUT_RELEASED, event.button.x, event.button.y);
		}

		bcase SDL_QUIT:
		{
			exitVal(0);
		}
	}
}

#ifdef CONFIG_ENV_WEBOS
	const char *storagePath() { return "/media/internal"; }
#endif

}

int main(int argc, char** argv)
{
	using namespace Base;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER
	#ifdef CONFIG_AUDIO_SDL
			| SDL_INIT_AUDIO
	#endif
	);
	#ifdef CONFIG_BASE_SDL_PDL
		PDL_Init(0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
		PDL_ScreenMetrics metrics;
		PDL_GetScreenMetrics(&metrics);
		logMsg("screen metrics from device: %dx%d pixels %dx%d DPI %f AR", metrics.horizontalPixels, metrics.verticalPixels,
				metrics.horizontalDPI, metrics.verticalDPI, metrics.aspectRatio);
		setupScreenSizeFromDevice(metrics);
		newXSize = metrics.horizontalPixels;
		newYSize = metrics.verticalPixels;
		logMsg("screen size in MM %dx%d", Gfx::viewMMWidth_, Gfx::viewMMHeight_);
		PDL_SetTouchAggression(PDL_AGGRESSION_MORETOUCHES);
	#else
		//TODO:
		Gfx::viewMMWidth_ = 100;
		Gfx::viewMMHeight_ = 100;
		newXSize = 320;
		newYSize = 480;
	#endif
	globalArgc = argc;
	globalArgv = argv;

	logger_init();

	#if defined(CONFIG_FS) && !defined(CONFIG_ENV_WEBOS)
		Fs::changeToAppDir(argv[0]);
	#endif

	#ifdef CONFIG_ENV_WEBOS
		appPath = getcwd(0, 0);
	#endif

	engineInit();
	//SDL_WM_SetCaption("SDL", "SDL");

	for(;;)
	{
		SDL_Event event;
		#ifdef CONFIG_ENV_WEBOS // halt screen updates when app is carded until event requests update
		if(appState != APP_RUNNING)
			gfxUpdate = 0;
		#endif
		while((gfxUpdate && SDL_PollEvent(&event) == 1) || !gfxUpdate)
		{
			if(!gfxUpdate)
			{
				//logMsg("sleeping until event");
				SDL_WaitEvent(&event);
			}
			eventHandler(event);
		}

		//logMsg("running gfx update");
		runEngine();
	}

	return 0;
}

#undef thisModuleName
