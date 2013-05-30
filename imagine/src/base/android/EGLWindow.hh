#pragma once

#include <android/window.h>
#include <logger/interface.h>
#include <engine-globals.h>
#include <util/egl.hh>

struct EGLWindow
{
	EGLDisplay display = EGL_NO_DISPLAY;
	EGLSurface surface = EGL_NO_SURFACE;
	EGLContext context = EGL_NO_CONTEXT;
	EGLConfig config = nullptr;
	bool useMaxColorBits = 0;
	bool has32BppColorBugs = 0;

	constexpr EGLWindow() { }

	void initEGL()
	{
		logMsg("doing EGL init");
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		assert(display != EGL_NO_DISPLAY);
		eglInitialize(display, 0, 0);

		//printEGLConfs(display);
		//printEGLConfsWithAttr(display, eglAttrWinMaxRGBA);
		//printEGLConfsWithAttr(display, eglAttrWinRGB888);
		//printEGLConfsWithAttr(display, eglAttrWinLowColor);

		logMsg("%s (%s), extensions: %s", eglQueryString(display, EGL_VENDOR), eglQueryString(display, EGL_VERSION), eglQueryString(display, EGL_EXTENSIONS));
	}

	void chooseConfig()
	{
		EGLint configs = 0;
		if(useMaxColorBits)
		{
			// try RGB and search for non-alpha RGB 888 config
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

	void initContext(ANativeWindow *win)
	{
		chooseConfig();
		#ifndef NDEBUG
		printEGLConf(display, config);
		#endif

		assert(context == EGL_NO_CONTEXT);

		logMsg("creating GL context");
		context = eglCreateContext(display, config, 0, 0);

		initSurface(win);
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
			logWarn("config didn't provide a native format id, guessing %d", nId);
		}
		return nId;
	}

	int currentWindowFormat()
	{
		assert(display != EGL_NO_DISPLAY);
		return winFormatFromConfig(display, config);
	}

	void initSurface(ANativeWindow *win)
	{
		assert(display != EGL_NO_DISPLAY);
		int configFormat = winFormatFromConfig(display, config);
		#ifndef NDEBUG
		int currFormat = ANativeWindow_getFormat(win);
		if(currFormat != configFormat)
		{
			logMsg("changing window format from %d to %d", currFormat, configFormat);
		}
		#endif
		ANativeWindow_setBuffersGeometry(win, 0, 0, configFormat);
		logMsg("current window format: %d", ANativeWindow_getFormat(win));

		assert(context != EGL_NO_CONTEXT);

		assert(surface == EGL_NO_SURFACE);
		logMsg("creating window surface");
		surface = eglCreateWindowSurface(display, config, win, 0);

		if(eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		{
			logErr("error in eglMakeCurrent");
			abort();
		}
		logMsg("window size: %d,%d, from EGL: %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win), width(), height());

		/*if(eglSwapInterval(display, 1) != EGL_TRUE)
		{
			logErr("error in eglSwapInterval");
		}*/
	}

	EGLint width()
	{
		assert(surface != EGL_NO_SURFACE);
		assert(display != EGL_NO_DISPLAY);
		EGLint w;
		eglQuerySurface(display, surface, EGL_WIDTH, &w);
		return w;
	}

	EGLint height()
	{
		assert(surface != EGL_NO_SURFACE);
		assert(display != EGL_NO_DISPLAY);
		EGLint h;
		eglQuerySurface(display, surface, EGL_HEIGHT, &h);
		return h;
	}

	void destroySurface()
	{
		if(isDrawable())
		{
			logMsg("destroying window surface");
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroySurface(display, surface);
			surface = EGL_NO_SURFACE;
		}
	}

	void destroyContext()
	{
		destroySurface();
		logMsg("destroying GL context");
		assert(surface == EGL_NO_SURFACE);
		assert(context != EGL_NO_CONTEXT);
		eglDestroyContext(display, context);
		context = EGL_NO_CONTEXT;
	}

	bool verifyContext()
	{
		return eglGetCurrentContext() != EGL_NO_CONTEXT;
	}

	void restoreContext()
	{
		assert(context);
		if(!verifyContext())
		{
			//logMsg("context not current, setting now");
			if(eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
			{
				logErr("error in eglMakeCurrent");
			}
		}
	}

	bool isDrawable()
	{
		return surface != EGL_NO_SURFACE;
	}

	void swap()
	{
		//auto now = TimeSys::timeNow();
		eglSwapBuffers(display, surface);
		//auto after = TimeSys::timeNow();
		//logMsg("swap time %f", double(after-now));
	}
};
