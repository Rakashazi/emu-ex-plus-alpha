#pragma once

#define Time X11Time_
#define Pixmap X11Pixmap_
#define GC X11GC_
#define Window X11Window
#define BOOL X11BOOL
#include <GL/glx.h>
#undef Time
#undef Pixmap
#undef GC
#undef Window
#undef BOOL

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

class GlxContext
{
	GLXContext ctx = nullptr;
	Display *dpy = nullptr;
	X11Window win {0};
	bool doubleBuffered = 0;
	int (*glXSwapIntervalSGI)(int interval) = nullptr;
	int (*glXSwapIntervalMESA)(unsigned int interval) = nullptr;

	bool createContext(XVisualInfo *vi)
	{
		int glxMajorVersion, glxMinorVersion;
		glXQueryVersion(dpy, &glxMajorVersion, &glxMinorVersion);
		logMsg("GLX version %d.%d", glxMajorVersion, glxMinorVersion);

		ctx = glXCreateContext(dpy, vi, 0, GL_TRUE);

		if (glXIsDirect(dpy, ctx))
			logMsg("using direct rendering");
		else
		{
			logMsg("direct rendering not supported, check your X configuration");
			return 0;
		}

		return 1;
	}

public:
	constexpr GlxContext() { }
	bool useMaxColorBits = 1;

	void makeCurrent()
	{
		assert(ctx);
		glXMakeCurrent(dpy, win, ctx);
	}

	void swap()
	{
		if(likely(doubleBuffered))
			glXSwapBuffers(dpy, win);
	}

	X11Window init(Display *dpy, int screen, uint xres, uint yres, bool multisample, long event_mask)
	{
		if(!useMaxColorBits)
		{
			logMsg("requesting lowest color config");
		}

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

		int *config = useMaxColorBits ? (multisample ? rgbDoubleMultisample : rgbDouble)
			: (multisample ? rgbLowDoubleMultisample : rgbLowDouble);

		var_selfs(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, screen, config);
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

		Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
		XSetWindowAttributes attr = { 0 };
		attr.colormap = cmap;
		attr.event_mask = event_mask;

		win = XCreateWindow(dpy, RootWindow(dpy, vi->screen),
				0, 0, xres, yres, 0, vi->depth, InputOutput, vi->visual,
				CWColormap | CWEventMask, &attr);

		if(!win || !createContext(vi))
		{
			XFree(vi);
			return 0;
		}

		XFree(vi);

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
		return win;
	}

	void setSwapInterval(uint interval)
	{
		assert(interval > 0);

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

	void deinit()
	{
		if(!glXMakeCurrent(dpy, None, 0))
		{
			logErr("could not release drawing context");
		}
		glXDestroyContext(dpy, ctx);
		ctx = 0;
	}
};
