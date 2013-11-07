#pragma once

#include <android/window.h>
#include <logger/interface.h>
#include <engine-globals.h>
#include <util/egl.hh>

struct EGLContextHelper
{
	EGLContext context = EGL_NO_CONTEXT;
	EGLConfig config = nullptr;
	bool useMaxColorBits = Config::MACHINE_IS_OUYA;
	bool has32BppColorBugs = 0;

	constexpr EGLContextHelper() {}

	void chooseConfig(EGLDisplay display)
	{
		EGLint configs = 0;
		if(useMaxColorBits)
		{
			// search for non-alpha RGB 888 config
			EGLConfig configRGBX[8];
			eglChooseConfig(display, eglAttrWinRGB888, configRGBX, sizeofArray(configRGBX), &configs);
			if(!configs)
			{
				logMsg("No 24-bit color configs found, using lowest color config");
				eglChooseConfig(display, eglAttrWinLowColor, &config, 1, &configs);
				assert(configs);
				return;
			}
			bool gotRGBXConfig = 0;
			iterateTimes(configs, i)
			{
				EGLint alphaSize;
				eglGetConfigAttrib(display, configRGBX[i], EGL_ALPHA_SIZE, &alphaSize);
				if(!alphaSize)
				{
					config = configRGBX[i];
					gotRGBXConfig = 1;
					break;
				}
			}
			if(!gotRGBXConfig)
			{
				logMsg("no RGBX configs, using RGBA instead");
				eglChooseConfig(display, eglAttrWinMaxRGBA, &config, 1, &configs);
			}
		}
		else
		{
			logMsg("requesting lowest color config");
			eglChooseConfig(display, eglAttrWinLowColor, &config, 1, &configs);
		}
		assert(configs);
	}

	void init(EGLDisplay display)
	{
		assert(!isInit());
		logMsg("creating GL context");
		context = eglCreateContext(display, config, 0, 0);
	}

	static int winFormatFromConfig(EGLDisplay display, EGLConfig config)
	{
		EGLint nId;
		eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &nId);
		if(!nId)
		{
			nId = WINDOW_FORMAT_RGBA_8888;
			EGLint alphaSize;
			eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &alphaSize);
			if(!alphaSize)
				nId = WINDOW_FORMAT_RGBX_8888;
			EGLint redSize;
			eglGetConfigAttrib(display, config, EGL_RED_SIZE, &redSize);
			if(redSize < 8)
				nId = WINDOW_FORMAT_RGB_565;
			//logWarn("config didn't provide a native format id, guessing %d", nId);
		}
		return nId;
	}

	int currentWindowFormat(EGLDisplay display)
	{
		assert(display != EGL_NO_DISPLAY);
		return winFormatFromConfig(display, config);
	}

	/*void destroyContext(EGLDisplay display)
	{
		destroySurface(display);
		logMsg("destroying GL context");
		assert(surface == EGL_NO_SURFACE);
		assert(context != EGL_NO_CONTEXT);
		eglDestroyContext(display, context);
		context = EGL_NO_CONTEXT;
	}*/

	bool isInit() const
	{
		return context != EGL_NO_CONTEXT;
	}

	bool verify()
	{
		return eglGetCurrentContext() != EGL_NO_CONTEXT;
	}

	void restore(EGLDisplay display, EGLSurface surface)
	{
		assert(isInit());
		if(!verify())
		{
			//logMsg("context not current, setting now");
			if(eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
			{
				logErr("error in eglMakeCurrent");
			}
		}
	}
};
