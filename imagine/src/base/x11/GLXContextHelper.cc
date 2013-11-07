#include "GLContextHelper.hh"

// TODO: remove GLX implementation once there is sufficient driver support from desktop GPU vendors for EGL

namespace Base
{

// single buffered visual
static int rgbSingle[] =
{
	GLX_RGBA,
	GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1,  GLX_BLUE_SIZE, 1,
	//GLX_DEPTH_SIZE, 16,
	None
};

// double buffered visual
static int rgbDouble[] =
{
	GLX_RGBA,
	GLX_DOUBLEBUFFER,
	GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
	//GLX_DEPTH_SIZE, 16,
	None
};

static int rgbDoubleMultisample[] =
{
	GLX_RGBA,
	GLX_DOUBLEBUFFER,
	GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
	//GLX_DEPTH_SIZE, 16,
	GLX_SAMPLE_BUFFERS_ARB, 1,
	GLX_SAMPLES_ARB, 4,
	None
};

// low-color, single buffered visual
static int rgbLowSingle[] =
{
	GLX_RGBA,
	None
};

// low-color, double buffered visual
static int rgbLowDouble[] =
{
	GLX_RGBA,
	GLX_DOUBLEBUFFER,
	None
};

static int rgbLowDoubleMultisample[] =
{
	GLX_RGBA,
	GLX_DOUBLEBUFFER,
	GLX_SAMPLE_BUFFERS_ARB, 1,
	GLX_SAMPLES_ARB, 4,
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

CallResult GLContextHelper::init(Display *dpy, int screen, bool multisample)
{
	assert(!ctx);
	if(!useMaxColorBits)
	{
		logMsg("requesting lowest color config");
	}

	int *config = useMaxColorBits ? (multisample ? rgbDoubleMultisample : rgbDouble)
		: (multisample ? rgbLowDoubleMultisample : rgbLowDouble);

	vi = glXChooseVisual(dpy, screen, config);
	if(!vi)
	{
		if(multisample)
		{
			return INVALID_PARAMETER;
		}
		vi = glXChooseVisual(dpy, screen, useMaxColorBits ? rgbSingle : rgbLowSingle);
		doubleBuffered = 0;
		logMsg("using single-buffered visual");
	}
	else
	{
		doubleBuffered = 1;
		logMsg("using double-buffered visual");
	}
	#ifndef NDEBUG
	printGLXVisual(dpy, vi);
	#endif

	ctx = glXCreateContext(dpy, vi, 0, GL_TRUE);

	#ifndef NDEBUG
	int glxMajorVersion, glxMinorVersion;
	glXQueryVersion(dpy, &glxMajorVersion, &glxMinorVersion);
	logMsg("GLX version %d.%d", glxMajorVersion, glxMinorVersion);

	if(glXIsDirect(dpy, ctx))
		logMsg("using direct rendering");
	else
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
