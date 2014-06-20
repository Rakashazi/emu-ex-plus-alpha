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

#include <imagine/base/GLContext.hh>
#include <imagine/logger/logger.h>
#include "x11.hh"

// TODO: remove GLX implementation once there is sufficient driver support from desktop GPU vendors for EGL

namespace Base
{

struct GLXFBConfigWrapper
{
	GLXFBConfig config;
	bool isValid = false;
};

using GLXAttrList = StaticArrayList<int, 24>;

static GLXAttrList glConfigAttrsToGLXAttrs(const GLConfigAttributes &attr)
{
	GLXAttrList list;

	list.push_back(GLX_DRAWABLE_TYPE);
	list.push_back(GLX_WINDOW_BIT|GLX_PBUFFER_BIT);

	list.push_back(GLX_DOUBLEBUFFER);
	list.push_back(True);

	if(attr.preferredColorBits() > 16)
	{
		list.push_back(GLX_RED_SIZE);
		list.push_back(1);
		list.push_back(GLX_GREEN_SIZE);
		list.push_back(1);
		list.push_back(GLX_BLUE_SIZE);
		list.push_back(1);
	}
	else
	{
		logMsg("requesting lowest color config");
	}

	list.push_back(None);
	return list;
}

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

static GLXContext createContextForMajorVersion(uint version, Display *dpy, GLXFBConfig config, bool &supportsSurfaceless)
{
	supportsSurfaceless = false;
	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
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
			{
				supportsSurfaceless = true;
				return ctx;
			}
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
			{
				supportsSurfaceless = true;
				return ctx;
			}
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
			{
				supportsSurfaceless = true;
				return ctx;
			}
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

CallResult GLContext::init(const GLConfigAttributes &attr)
{
	assert(!context);
	display = dpy;
	int screenIdx = indexOfScreen(mainScreen());
	GLXFBConfigWrapper fbc;
	{
		// TODO: check if useMaxColorBits configs actually pick lowest color mode
		auto glxAttr = glConfigAttrsToGLXAttrs(attr);
		fbc = chooseFBConfig(display, screenIdx, &glxAttr[0]);
	}
	if(!fbc.isValid)
	{
		return INVALID_PARAMETER;
	}
	vi = glXGetVisualFromFBConfig(display, fbc.config);
	if(!vi)
	{
		logErr("failed to get matching visual for fbconfig");
		return INVALID_PARAMETER;
	}
	#ifndef NDEBUG
	printGLXVisual(display, vi);
	#endif

	bool supportsSurfaceless;
	context = createContextForMajorVersion(attr.majorVersion(), display, fbc.config, supportsSurfaceless);
	if(!context)
	{
		logErr("can't find a compatible context");
		return INVALID_PARAMETER;
	}
	supportsSurfaceless = false; // TODO: glXMakeContextCurrent doesn't work correctly
	if(!supportsSurfaceless)
	{
		const int attrib[] {GLX_PBUFFER_WIDTH, 1, GLX_PBUFFER_HEIGHT, 1, None};
		dummyPbuff = glXCreatePbuffer(display, fbc.config, attrib);
		if(!dummyPbuff)
		{
			bug_exit("couldn't make dummy pbuffer");
		}
	}
	setCurrent(this, nullptr);

	#ifndef NDEBUG
	int glxMajorVersion, glxMinorVersion;
	glXQueryVersion(display, &glxMajorVersion, &glxMinorVersion);
	logMsg("GLX version %d.%d, direct context: %s", glxMajorVersion, glxMinorVersion, glXIsDirect(display, context) ? "yes" : "no");
	if(!glXIsDirect(display, context))
		bug_exit("direct rendering not supported, check your X configuration");
	#endif

	return OK;
}

GLConfig GLContext::bufferConfig()
{
	return GLConfig{vi};
}

void XGLContext::setCurrentContext(XGLContext *c, Window *win)
{
	if(!c)
	{
		logMsg("setting no context current");
		if(!glXMakeContextCurrent(dpy, None, None, nullptr))
		{
			bug_exit("error setting no context current");
		}
	}
	else if(win)
	{
		if(!glXMakeContextCurrent(c->display, win->xWin, win->xWin, c->context))
		{
			bug_exit("error setting window 0x%p current", win);
		}
	}
	else
	{
		// TODO: with nvidia (tested on 337.12) glXMakeContextCurrent fails with badmatch
		// even if using a 3.0 context
		/*if(!c->dummyPbuff)
		{
			logMsg("setting no drawable current");
			if(!glXMakeContextCurrent(c->display, None, None, c->context))
			{
				bug_exit("error setting no drawable current");
			}
		}
		else*/
		{
			logMsg("setting dummy pbuffer current");
			if(!glXMakeContextCurrent(c->display, c->dummyPbuff, c->dummyPbuff, c->context))
			{
				bug_exit("error setting dummy pbuffer current");
			}
		}
	}
}

void XGLContext::setCurrentDrawable(Window *win)
{
	setCurrentContext(this, win);
}

bool XGLContext::isRealCurrentContext()
{
	return glXGetCurrentContext() == context;
}

void GLContext::present(Window &win)
{
	glXSwapBuffers(display, win.xWin);
}

GLContext::operator bool() const
{
	return context;
}

void GLContext::deinit()
{
	if(context)
	{
		if(current() == this)
		{
			GLContext::setCurrent(nullptr, nullptr);
		}
		glXDestroyContext(display, context);
		context = nullptr;
	}
	if(vi)
	{
		XFree(vi);
		vi = nullptr;
	}
}

}
