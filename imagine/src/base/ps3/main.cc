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

#define thisModuleName "base:ps3"
#include <base/Base.hh>
#include <gfx/Gfx.hh>
#include <base/common/funcs.h>

#include <stdio.h>
#include <PSGL/psgl.h>

#include <sys/process.h>
#include <sys/spu_initialize.h>
#include <sys/paths.h>
#include <cell/sysmodule.h>
#include <sys/timer.h>
#include <sysutil/sysutil_sysparam.h>

#ifdef CONFIG_FS_PS3
#include <fs/ps3/FsPs3.hh>
#endif

typedef void (*Func)(void);
CLINK Func* __ctors64_start;
CLINK Func* __ctors64_end;

CallResult logger_ps3_init(uint w, uint h);

namespace Input
{
	void update();
}

void base_abort()
{
	Base::abort();
}

namespace Base
{

const char *appPath = 0;
uint appState = APP_RUNNING;

static void updateFrame()
{
	logger_update();
	psglSwap();
}

void openGLUpdateScreen()
{
	updateFrame();
}

static void exitApp() ATTRS(noreturn);
static void exitApp()
{
	appState = APP_EXITING;
	onExit(0);
	glFinish();
	::exit(0);
}

static void haltOnExit() ATTRS(noreturn);
static void haltOnExit()
{
	// stop when exit called so log can be viewed
	logMsg("halting app");
	while(1)
	{
		cellSysutilCheckCallback();
		updateFrame();
	}
}

void exitVal(int returnVal) { exitApp(); }
void abort() { haltOnExit(); }

/*static int getResolutionWidthHeight(const unsigned int resolutionId, unsigned int &w, unsigned int &h)
{
	switch(resolutionId)
	{
		case CELL_VIDEO_OUT_RESOLUTION_480       : w=720;  h=480;  return(1);
		case CELL_VIDEO_OUT_RESOLUTION_576       : w=720;  h=576;  return(1);
		case CELL_VIDEO_OUT_RESOLUTION_720       : w=1280; h=720;  return(1);
		case CELL_VIDEO_OUT_RESOLUTION_1080      : w=1920; h=1080; return(1);
		case CELL_VIDEO_OUT_RESOLUTION_1600x1080 : w=1600; h=1080; return(1);
		case CELL_VIDEO_OUT_RESOLUTION_1440x1080 : w=1440; h=1080; return(1);
		case CELL_VIDEO_OUT_RESOLUTION_1280x1080 : w=1280; h=1080; return(1);
		case CELL_VIDEO_OUT_RESOLUTION_960x1080  : w=960;  h=1080; return(1);
	};
	printf("getResolutionWidthHeight: resolutionId %d not a valid video mode\n", resolutionId);
	return(0);
}

static int chooseBestResolution(const unsigned int *resolutions, unsigned int numResolutions)
{
	iterateTimes(numResolutions, i)
	{
		if(cellVideoOutGetResolutionAvailability(CELL_VIDEO_OUT_PRIMARY, resolutions[i], CELL_VIDEO_OUT_ASPECT_AUTO, 0))
		{
			return resolutions[i];
		}
	}
	// fall back to 480
	return CELL_VIDEO_OUT_RESOLUTION_480;
}*/

void setupPSGL()
{
	PSGLinitOptions initOpts =
	{
		enable: PSGL_INIT_MAX_SPUS | PSGL_INIT_INITIALIZE_SPUS | PSGL_INIT_HOST_MEMORY_SIZE,
		maxSPUs: 1,
		initializeSPUs: false,
		persistentMemorySize: 0,
		transientMemorySize: 0,
		errorConsole: 0,
		fifoSize: 0,
		hostMemorySize: 128 * 1024 * 1024,  // 128MB host memory
	};
	psglInit(&initOpts);

	//const unsigned int resolutions[] = { CELL_VIDEO_OUT_RESOLUTION_1080, CELL_VIDEO_OUT_RESOLUTION_720, CELL_VIDEO_OUT_RESOLUTION_576, CELL_VIDEO_OUT_RESOLUTION_480 };
	//int bestResolution = chooseBestResolution(resolutions, sizeofArray(resolutions));

	//getResolutionWidthHeight(bestResolution, deviceWidth, deviceHeight);

    /*PSGLdeviceParameters params;
    params.enable = PSGL_DEVICE_PARAMETERS_COLOR_FORMAT | PSGL_DEVICE_PARAMETERS_DEPTH_FORMAT | PSGL_DEVICE_PARAMETERS_MULTISAMPLING_MODE;
    params.colorFormat = GL_ARGB_SCE;
    params.depthFormat = GL_DEPTH_COMPONENT24;
    params.multisamplingMode = GL_MULTISAMPLING_NONE_SCE;

    params.enable |= PSGL_DEVICE_PARAMETERS_WIDTH_HEIGHT;
    params.width = deviceWidth;
    params.height = deviceHeight;

    PSGLdevice *device = psglCreateDeviceExtended(&params);*/
	PSGLdevice *device = psglCreateDeviceAuto(GL_ARGB_SCE, /*GL_DEPTH_COMPONENT24*/ GL_NONE, GL_MULTISAMPLING_NONE_SCE);

	unsigned int deviceWidth = 0, deviceHeight = 0;
	psglGetDeviceDimensions(device, &deviceWidth, &deviceHeight);
	//gfxAspectRatio = psglGetDeviceAspectRatio(device);

	PSGLcontext *context = psglCreateContext();
	psglMakeCurrent(context, device);
	/*char shaderPath[CELL_FS_MAX_FS_PATH_LENGTH];
	sprintf(shaderPath, "%s/shaders.bin", FsPs3::workDir());
	psglLoadShaderLibrary(shaderPath);*/
	psglResetCurrentContext();

	logger_ps3_init(deviceWidth, deviceHeight);
	using namespace Gfx;
	viewPixelWidth_ = mainWin.w = mainWin.rect.x2 = deviceWidth;
	viewPixelHeight_ = mainWin.h = mainWin.rect.y2 = deviceHeight;
	logMsg("init screen %dx%d", deviceWidth, deviceHeight);
	viewMMHeight_ = 330;
	if(deviceHeight == 720 || deviceHeight == 1080)
		viewMMWidth_ = 586;
	else
		viewMMWidth_ = 440;

	glViewport(0, 0, deviceWidth, deviceHeight);
	glScissor(0, 0, deviceWidth, deviceHeight);
	glEnable(GL_VSYNC_SCE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static bool videoOutIsReady()
{
	CellVideoOutState videoState;
	cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);
	return( videoState.state==CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED );
}

void displayNeedsUpdate() { generic_displayNeedsUpdate(); }

void sleepUs(int us)
{
	sys_timer_usleep(us);
}

void sleepMs(int ms)
{
	sleepUs(ms*1000);
}

CallbackRef *callbackAfterDelay(CallbackDelegate callback, int ms)
{
	//TODO
	return nullptr;
}

void cancelCallback(CallbackRef *ref)
{
	//TODO
}

//static TimerCallbackFunc timerCallbackFunc = 0;
//static void *timerCallbackFuncCtx = 0;
//static uint timerCountDown = 0;
//void setTimerCallback(TimerCallbackFunc f, void *ctx, int ms)
//{
//	if(timerCallbackFunc)
//	{
//		logMsg("canceling callback");
//		timerCallbackFunc = 0;
//	}
//	if(!f)
//		return;
//	logMsg("setting callback to run in %d ms, %d frames", ms, ms / 16);
//	timerCallbackFunc = f;
//	timerCallbackFuncCtx = ctx;
//	timerCountDown = ms / 16;
//}

//void base_openURL(const char *url) { };

int main2()
{
	using namespace Base;

	auto funcarr = (Func*)&__ctors64_start;
	uint ctors = ((uint64)&__ctors64_end - (uint64)&__ctors64_start) / 8;
	iterateTimes(ctors, i)
	{
		//logMsg("ctor %d @ %p", i, funcarr[i]);
		funcarr[i]();
	}

	sys_spu_initialize(6, 1);
	while(!Base::videoOutIsReady())
	{
		// wait for video out
	};
	Base::setupPSGL();

	#ifdef CONFIG_FS_PS3
		cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
		FsPs3::initWorkDir();
	#endif

	#ifdef CONFIG_INPUT
	doOrExit(Input::init());
	#endif
	doOrExit(onInit(0, nullptr));
	Base::engineInit();
	logMsg("done init");

	while(1)
	{
		#ifdef CONFIG_INPUT
		Input::update();
		#endif
		cellSysutilCheckCallback();
		Base::gfxUpdate = 1; // update gfx constantly for now
		Base::runEngine(0);
		// TODO
//		if(unlikely(timerCallbackFunc != 0))
//		{
//			if(unlikely(timerCountDown == 0))
//			{
//				logMsg("running callback");
//				timerCallbackFunc(timerCallbackFuncCtx);
//				timerCallbackFunc = 0;
//			}
//			else
//				timerCountDown--;
//		}
	}
	return 0;
}

}
