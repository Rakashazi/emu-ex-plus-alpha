#pragma once

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

#include <android/window.h>
#include <imagine/logger/logger.h>
#include <imagine/engine-globals.h>
#include <imagine/util/egl.hh>
#include <imagine/gfx/Gfx.hh>

struct EGLContextHelper
{
	EGLContext context = EGL_NO_CONTEXT;
	EGLConfig config = nullptr;
	EGLSurface dummyPbuff = EGL_NO_SURFACE; // dummy pbuffer when no windows exist
	bool useMaxColorBits = Config::MACHINE_IS_OUYA;
	//bool has32BppColorBugs = false;
	#ifndef NDEBUG
	bool configIsSet = false;
	#endif

	constexpr EGLContextHelper() {}

	void chooseConfig(EGLDisplay display)
	{
		#ifndef NDEBUG
		assert(!configIsSet); // only call this function once
		configIsSet = true;
		#endif
		EGLint configs = 0;
		const EGLint *attrWinRGB888 = Gfx::maxOpenGLMajorVersionSupport() == 1 ? eglAttrWinRGB888 : eglAttrWinRGB888ES2;
		const EGLint *attrWinMaxRGBA = Gfx::maxOpenGLMajorVersionSupport() == 1 ? eglAttrWinMaxRGBA : eglAttrWinMaxRGBAES2;
		const EGLint *attrWinLowColor = Gfx::maxOpenGLMajorVersionSupport() == 1 ? eglAttrWinLowColor : eglAttrWinLowColorES2;
		if(useMaxColorBits)
		{
			// search for non-alpha RGB 888 config
			EGLConfig configRGBX[8];
			eglChooseConfig(display, attrWinRGB888, configRGBX, sizeofArray(configRGBX), &configs);
			if(!configs)
			{
				logMsg("No 24-bit color configs found, using lowest color config");
				eglChooseConfig(display, attrWinLowColor, &config, 1, &configs);
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
				eglChooseConfig(display, attrWinMaxRGBA, &config, 1, &configs);
			}
		}
		else
		{
			logMsg("requesting lowest color config");
			eglChooseConfig(display, attrWinLowColor, &config, 1, &configs);
		}
		assert(configs);
	}

	void init(EGLDisplay display)
	{
		logMsg("creating GL context");
		assert(!isInit());
		#ifndef NDEBUG
		assert(configIsSet);
		#endif
		if(Gfx::maxOpenGLMajorVersionSupport() == 1)
			context = eglCreateContext(display, config, 0, nullptr);
		else if(Gfx::maxOpenGLMajorVersionSupport() == 2)
			context = eglCreateContext(display, config, 0, eglAttrES2Ctx);
		else
			bug_exit("unsupported OpenGL ES major version: %d", Gfx::maxOpenGLMajorVersionSupport());
		bool supportsSurfaceless = strstr(eglQueryString(display, EGL_EXTENSIONS), "EGL_KHR_surfaceless_context");
		if(!supportsSurfaceless)
		{
			logMsg("surfaceless context not supported");
			dummyPbuff = makeDummyPbuffer(display, config);
			assert(dummyPbuff != EGL_NO_SURFACE);
		}
		makeCurrentSurfaceless(display);
	}

	void makeCurrentSurfaceless(EGLDisplay display)
	{
		assert(isInit());
		if(dummyPbuff != EGL_NO_SURFACE)
		{
			logMsg("setting dummy pbuffer surface current");
			if(eglMakeCurrent(display, dummyPbuff, dummyPbuff, context) == EGL_FALSE)
			{
				bug_exit("error setting dummy pbuffer current");
			}
		}
		else
		{
			logMsg("setting no surface current");
			if(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context) == EGL_FALSE)
			{
				bug_exit("error setting no surface current");
			}
		}
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
		#ifndef NDEBUG
		assert(configIsSet);
		#endif
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
		assert(isInit());
		return eglGetCurrentContext() == context;
	}
};
