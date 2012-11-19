#pragma once

#define Time X11Time_
#define Pixmap X11Pixmap_
#define GC X11GC_
#define Window X11Window
#define BOOL X11BOOL
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>
#undef Time
#undef Pixmap
#undef GC
#undef Window
#undef BOOL

class GlxContext
{
	GLXContext ctx = nullptr;
	Display *dpy = nullptr;
	X11Window win {0};
	bool doubleBuffered = 0, supportsSwapInterval = 0;

	bool createContext(XVisualInfo *vi)
	{
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
		// single buffered visual
		static int attrListSgl[] =
		{
			GLX_RGBA,
			GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4,  GLX_BLUE_SIZE, 4,
			//GLX_DEPTH_SIZE, 16,
			None
		};

		// double buffered visual
		static int attrListDbl[] =
		{
			GLX_RGBA,
			GLX_DOUBLEBUFFER,
			GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4,
			//GLX_DEPTH_SIZE, 16,
			None
		};

		static int attrListDblMultisample[] =
		{
			GLX_RGBA,
			GLX_DOUBLEBUFFER,
			GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4,
			//GLX_DEPTH_SIZE, 16,
			GLX_SAMPLE_BUFFERS_ARB, 1,
			GLX_SAMPLES_ARB, 4,
			None
		};

		var_selfs(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, screen, multisample ? attrListDblMultisample : attrListDbl);
		if(!vi)
		{
			if(multisample)
			{
				return INVALID_PARAMETER;
			}
			vi = glXChooseVisual(dpy, screen, attrListSgl);
			doubleBuffered = 0;
			logMsg("using single-buffered visual");
		}
		else
		{
			doubleBuffered = 1;
			logMsg("using double-buffered visual");
		}

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
			supportsSwapInterval = 1;
		}
		else
		{
			logWarn("no glXSwapIntervalSGI support");
		}
		return win;
	}

	void setSwapInterval(uint interval)
	{
		if(supportsSwapInterval)
		{
			assert(interval > 0);
			logMsg("set swap interval %d", interval);
			glXSwapIntervalSGI(interval);
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
