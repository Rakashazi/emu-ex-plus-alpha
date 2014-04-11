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

#include "GLContextHelper.hh"
#include <imagine/logger/logger.h>

// TODO: remove GLX implementation once there is sufficient driver support from desktop GPU vendors for EGL

namespace Base
{

// single buffered visual
static const int rgbSingle[] =
{
	GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1,  GLX_BLUE_SIZE, 1,
	//GLX_DEPTH_SIZE, 16,
	None
};

// double buffered visual
static const int rgbDouble[] =
{
	GLX_DOUBLEBUFFER, True,
	GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
	//GLX_DEPTH_SIZE, 16,
	None
};

static const int rgbDoubleMultisample[] =
{
	GLX_DOUBLEBUFFER, True,
	GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
	//GLX_DEPTH_SIZE, 16,
	GLX_SAMPLE_BUFFERS, 1,
	GLX_SAMPLES, 4,
	None
};

// low-color, single buffered visual
static const int rgbLowSingle[] =
{
	None
};

// low-color, double buffered visual
static const int rgbLowDouble[] =
{
	GLX_DOUBLEBUFFER, True,
	None
};

static const int rgbLowDoubleMultisample[] =
{
	GLX_DOUBLEBUFFER, True,
	GLX_SAMPLE_BUFFERS, 1,
	GLX_SAMPLES, 4,
	None
};

static const char* glxConfigCaveatToStr(int cav)
{
	switch(cav)
	{
		case GLX_NONE: return "none";
		case GLX_SLOW_CONFIG: return "slow";
		case GLX_NON_CONFORMANT_CONFIG: return "non-conformant";
	}
	return "unknown";
}

static void printGLXVisual(Display *display, XVisualInfo *config)
{
	int buffSize, redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize, sampleBuff;
	glXGetConfig(display, config, GLX_BUFFER_SIZE, &buffSize);
	glXGetConfig(display, config, GLX_RED_SIZE, &redSize);
	glXGetConfig(display, config, GLX_GREEN_SIZE, &greenSize);
	glXGetConfig(display, config, GLX_BLUE_SIZE, &blueSize);
	glXGetConfig(display, config, GLX_ALPHA_SIZE, &alphaSize);
	//glXGetConfig(display, config, GLX_CONFIG_CAVEAT, &cav);
	glXGetConfig(display, config, GLX_DEPTH_SIZE, &depthSize);
	glXGetConfig(display, config, GLX_STENCIL_SIZE, &stencilSize);
	glXGetConfig(display, config, GLX_SAMPLE_BUFFERS, &sampleBuff);
	logMsg("config %d %d:%d:%d:%d d:%d sten:%d sampleBuff:%d",
			buffSize, redSize, greenSize, blueSize, alphaSize,
			depthSize, stencilSize,
			sampleBuff);
}

GLContextHelper::operator bool() const
{
	return ctx;
}

struct GLXFBConfigWrapper
{
	GLXFBConfig config;
	bool isValid = false;
};

static GLXFBConfigWrapper chooseFBConfig(Display *dpy, int screen, const int *config)
{
	int count;
	GLXFBConfigWrapper fbc;
	GLXFBConfig *configPtr = glXChooseFBConfig(dpy, screen, config, &count);
	if(!configPtr)
	{
		return fbc;
	}
	fbc.config = *configPtr;
	fbc.isValid = true;
	XFree(configPtr);
	return fbc;
}

static GLXContext createContextForMajorVersion(uint version, Display *dpy, GLXFBConfig config)
{
	if(version >= 3)
	{
		{
			// Try 3.2 Core
			const int attrCore3_2[]
			{
				GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
				GLX_CONTEXT_MINOR_VERSION_ARB, 2,
				GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
				#ifndef NDEBUG
				GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
				#endif
				None
			};
			GLXContext ctx = glXCreateContextAttribsARB(dpy, config, 0, True, attrCore3_2);
			if(ctx)
				return ctx;
			logErr("failed creating 3.2 core context");
		}
		{
			// Try 3.1
			const int attr3_1[] =
			{
				GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
				GLX_CONTEXT_MINOR_VERSION_ARB, 1,
				#ifndef NDEBUG
				GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
				#endif
				None
			};
			GLXContext ctx = glXCreateContextAttribsARB(dpy, config, 0, True, attr3_1);
			if(ctx)
				return ctx;
			logErr("failed creating 3.1 context");
		}
		{
			// Try 3.0
			const int attr3_0[] =
			{
				GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
				GLX_CONTEXT_MINOR_VERSION_ARB, 0,
				#ifndef NDEBUG
				GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
				#endif
				None
			};
			GLXContext ctx = glXCreateContextAttribsARB(dpy, config, 0, True, attr3_0);
			if(ctx)
				return ctx;
			logErr("failed creating 3.0 context");
		}
		// Fallback to 1.2
	}
	const int attr[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 1,
		GLX_CONTEXT_MINOR_VERSION_ARB, 2,
		#ifndef NDEBUG
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
		#endif
		None
	};
	GLXContext ctx = glXCreateContextAttribsARB(dpy, config, 0, True, attr);
	if(ctx)
		return ctx;
	logErr("failed creating 1.2 context");
	return (GLXContext)0;
}

CallResult GLContextHelper::init(Display *dpy, int screen, bool multisample, uint version)
{
	assert(!ctx);
	if(!useMaxColorBits)
	{
		logMsg("requesting lowest color config");
	}

	// TODO: check if useMaxColorBits configs actually pick lowest color mode
	const int *config = useMaxColorBits ? (multisample ? rgbDoubleMultisample : rgbDouble)
		: (multisample ? rgbLowDoubleMultisample : rgbLowDouble);

	GLXFBConfigWrapper fbc = chooseFBConfig(dpy, screen, config);
	if(!fbc.isValid)
	{
		if(multisample)
		{
			return INVALID_PARAMETER;
		}
		fbc = chooseFBConfig(dpy, screen, useMaxColorBits ? rgbSingle : rgbLowSingle);
		if(!fbc.isValid)
		{
			logErr("no fbconfigs found");
			return INVALID_PARAMETER;
		}
		doubleBuffered = 0;
		logMsg("using single-buffered config");
	}
	else
	{
		doubleBuffered = 1;
		logMsg("using double-buffered config");
	}
	vi = glXGetVisualFromFBConfig(dpy, fbc.config);
	if(!vi)
	{
		logErr("failed to get matching visual for fbconfig");
		return INVALID_PARAMETER;
	}
	#ifndef NDEBUG
	printGLXVisual(dpy, vi);
	#endif

	ctx = createContextForMajorVersion(version, dpy, fbc.config);
	if(!ctx)
	{
		logErr("can't find a compatible context");
		return INVALID_PARAMETER;
	}

	#ifndef NDEBUG
	int glxMajorVersion, glxMinorVersion;
	glXQueryVersion(dpy, &glxMajorVersion, &glxMinorVersion);
	logMsg("GLX version %d.%d, direct context: %s", glxMajorVersion, glxMinorVersion, glXIsDirect(dpy, ctx) ? "yes" : "no");
	if(!glXIsDirect(dpy, ctx))
		bug_exit("direct rendering not supported, check your X configuration");
	#endif

	auto extensions = glXQueryExtensionsString(dpy, screen);
	if(strstr(extensions, "GLX_SGI_swap_control"))
	{
		glXSwapIntervalSGI = (int (*)(int))glXGetProcAddress((const GLubyte*) "glXSwapIntervalSGI");
		if(glXSwapIntervalSGI)
		{
			logMsg("has glXSwapIntervalSGI");
		}
	}
	else if(strstr(extensions, "GLX_MESA_swap_control"))
	{
		glXSwapIntervalMESA = (int (*)(unsigned int))glXGetProcAddress((const GLubyte*) "glXSwapIntervalMESA");
		if(glXSwapIntervalMESA)
		{
			logMsg("has glXSwapIntervalMESA");
		}
	}
	else
	{
		logWarn("no glXSwapInterval support");
	}

	return OK;
}

void GLContextHelper::makeCurrent(Display *dpy, const XWindow &win)
{
	assert(ctx);
	glXMakeCurrent(dpy, win.xWin, ctx);
}

void GLContextHelper::swap(Display *dpy, const XWindow &win)
{
	if(likely(doubleBuffered))
		glXSwapBuffers(dpy, win.xWin);
}

CallResult GLContextHelper::initWindowSurface(XWindow &win)
{
	// nothing to do
	return OK;
}

void GLContextHelper::deinitWindowSurface(XWindow &win)
{
	// nothing to do
}

void GLContextHelper::setSwapInterval(uint interval)
{
	if(glXSwapIntervalSGI || glXSwapIntervalMESA)
		logMsg("set swap interval %d", interval);

	if(glXSwapIntervalSGI)
	{
		glXSwapIntervalSGI(interval);
	}
	else if(glXSwapIntervalMESA)
	{
		glXSwapIntervalMESA(interval);
	}
}

void GLContextHelper::deinit(Display *dpy)
{
	if(!glXMakeCurrent(dpy, None, nullptr))
	{
		logWarn("could not release drawing context");
	}
	if(vi)
	{
		XFree(vi);
		vi = nullptr;
	}
	if(ctx)
	{
		glXDestroyContext(dpy, ctx);
		ctx = nullptr;
	}
}

}
