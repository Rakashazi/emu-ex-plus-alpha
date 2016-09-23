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

#define LOGTAG "GLX"
#include <imagine/base/GLContext.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include "internal.hh"

// TODO: remove GLX implementation once there is sufficient driver support from desktop GPU vendors for EGL

namespace Base
{

static bool hasDummyPbuffConfig = false;
static GLXFBConfig dummyPbuffConfig{};
using GLXAttrList = StaticArrayList<int, 24>;
using GLXContextAttrList = StaticArrayList<int, 16>;

static GLXAttrList glConfigAttrsToGLXAttrs(GLBufferConfigAttributes attr)
{
	GLXAttrList list;

	list.push_back(GLX_DOUBLEBUFFER);
	list.push_back(True);

	if(attr.pixelFormat() == PIXEL_RGBA8888)
	{
		list.push_back(GLX_RED_SIZE);
		list.push_back(8);
		list.push_back(GLX_GREEN_SIZE);
		list.push_back(8);
		list.push_back(GLX_BLUE_SIZE);
		list.push_back(8);
		list.push_back(GLX_ALPHA_SIZE);
		list.push_back(8);
	}
	else
	{
		//logMsg("requesting lowest color config");
	}

	list.push_back(None);
	return list;
}

static GLXContextAttrList glContextAttrsToGLXAttrs(GLContextAttributes attr)
{
	GLXContextAttrList list;

	list.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
	list.push_back(attr.majorVersion());
	list.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
	list.push_back(attr.minorVersion());

	if(attr.majorVersion() > 3
		|| (attr.majorVersion() == 3 && attr.minorVersion() >= 2))
	{
		list.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
		list.push_back(GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
	}

	if(attr.debug())
	{
		list.push_back(GLX_CONTEXT_FLAGS_ARB);
		list.push_back(GLX_CONTEXT_DEBUG_BIT_ARB);
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

static void printGLXVisual(Display *display, XVisualInfo &config)
{
	int buffSize, redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize, sampleBuff;
	glXGetConfig(display, &config, GLX_BUFFER_SIZE, &buffSize);
	glXGetConfig(display, &config, GLX_RED_SIZE, &redSize);
	glXGetConfig(display, &config, GLX_GREEN_SIZE, &greenSize);
	glXGetConfig(display, &config, GLX_BLUE_SIZE, &blueSize);
	glXGetConfig(display, &config, GLX_ALPHA_SIZE, &alphaSize);
	//glXGetConfig(display, &config, GLX_CONFIG_CAVEAT, &cav);
	glXGetConfig(display, &config, GLX_DEPTH_SIZE, &depthSize);
	glXGetConfig(display, &config, GLX_STENCIL_SIZE, &stencilSize);
	glXGetConfig(display, &config, GLX_SAMPLE_BUFFERS, &sampleBuff);
	logMsg("visual:0x%x config %d %d:%d:%d:%d d:%d sten:%d sampleBuff:%d",
			(int)config.visualid, buffSize, redSize, greenSize, blueSize, alphaSize,
			depthSize, stencilSize,
			sampleBuff);
}

// GLDisplay

GLDisplay GLDisplay::makeDefault(std::error_code &ec)
{
	ec = {};
	return {dpy};
}

GLDisplay::operator bool() const
{
	return display;
}

bool GLDisplay::operator ==(GLDisplay const &rhs) const
{
	return display == rhs.display;
}

bool GLDisplay::deinit()
{
	display = {};
	return true;
}

GLDrawable GLDisplay::makeDrawable(Window &win, GLBufferConfig config, std::error_code &ec)
{
	ec = {};
	return {win.nativeObject()};
}

bool GLDisplay::deleteDrawable(GLDrawable &drawable)
{
	drawable = {};
	return true;
}

// GLDrawable

void GLDrawable::freeCaches() {}

GLDrawable::operator bool() const
{
	return drawable_;
}

bool GLDrawable::operator ==(GLDrawable const &rhs) const
{
	return drawable_ == rhs.drawable_;
}

// GLContext

GLBufferConfig GLContext::makeBufferConfig(GLDisplay display, GLContextAttributes, GLBufferConfigAttributes attr)
{
	GLBufferConfig conf;
	// force default pixel format for now since SGIFrameTimer needs context with matching format
	attr.setPixelFormat(Window::defaultPixelFormat());
	int screenIdx = indexOfScreen(mainScreen());
	{
		int count;
		auto glxAttr = glConfigAttrsToGLXAttrs(attr);
		auto fbcArr = glXChooseFBConfig(dpy, screenIdx, &glxAttr[0], &count);
		if(!fbcArr)
		{
			return GLBufferConfig{};
		}
		conf.glConfig = fbcArr[0];
		XFree(fbcArr);
	}
	{
		auto viPtr = glXGetVisualFromFBConfig(dpy, conf.glConfig);
		if(!viPtr)
		{
			logErr("failed to get matching visual for fbconfig");
			return GLBufferConfig{};
		}
		if(Config::DEBUG_BUILD)
			printGLXVisual(dpy, *viPtr);
		conf.fmt.visual = viPtr->visual;
		conf.fmt.depth = viPtr->depth;
		XFree(viPtr);
	}
	return conf;
}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, std::error_code &ec)
{
	logMsg("making context with version: %d.%d", attr.majorVersion(), attr.minorVersion());
	auto glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
	context = glXCreateContextAttribsARB(dpy, config.glConfig, 0, True, &glContextAttrsToGLXAttrs(attr)[0]);
	if(!context)
	{
		logErr("can't find a compatible context");
		ec = {EINVAL, std::system_category()};
		return;
	}
	bool supportsSurfaceless = false;
	if(!supportsSurfaceless)
	{
		if(!hasDummyPbuffConfig)
		{
			logMsg("surfaceless context not supported");
			dummyPbuffConfig = config.glConfig;
			hasDummyPbuffConfig = true;
		}
		else
		{
			// all contexts must use same config if surfaceless isn't supported
			assert(dummyPbuffConfig == config.glConfig);
		}
	}
	if(Config::DEBUG_BUILD)
	{
		int glxMajorVersion, glxMinorVersion;
		glXQueryVersion(dpy, &glxMajorVersion, &glxMinorVersion);
		logMsg("GLX version %d.%d, direct context: %s", glxMajorVersion, glxMinorVersion, glXIsDirect(dpy, context) ? "yes" : "no");
	}
	ec = {};
}

static void setCurrentContext(GLDisplay display, GLXContext context, GLDrawable win)
{
	if(!context)
	{
		logMsg("setting no context current");
		assert(!win);
		if(!glXMakeContextCurrent(dpy, None, None, nullptr))
		{
			bug_exit("error setting no context current");
		}
	}
	else if(win)
	{
		if(!glXMakeContextCurrent(dpy, win.drawable(), win.drawable(), context))
		{
			bug_exit("error setting window %lld current", (long long)win.drawable());
		}
	}
	else
	{
		if(hasDummyPbuffConfig)
		{
			logMsg("setting dummy pbuffer surface current");
			const int pbuffAttr[]{GLX_PBUFFER_WIDTH, 1, GLX_PBUFFER_HEIGHT, 1, None};
			auto dummyPbuff = glXCreatePbuffer(dpy, dummyPbuffConfig, pbuffAttr);
			if(!glXMakeContextCurrent(dpy, dummyPbuff, dummyPbuff, context))
			{
				bug_exit("error setting dummy pbuffer current");
			}
			glXDestroyPbuffer(dpy, dummyPbuff);
		}
		else
		{
			logMsg("setting no drawable current");
			if(!glXMakeContextCurrent(dpy, None, None, context))
			{
				bug_exit("error setting no drawable current");
			}
		}
	}
}

void GLContext::setCurrent(GLDisplay display, GLContext context, GLDrawable win)
{
	setCurrentContext(display, context.context, win);
}

void GLContext::setDrawable(GLDisplay display, GLDrawable win)
{
	setDrawable(display, win, current(display));
}

void GLContext::setDrawable(GLDisplay display, GLDrawable win, GLContext cachedCurrentContext)
{
	setCurrentContext(display, cachedCurrentContext.context, win);
}

GLContext GLContext::current(GLDisplay display)
{
	GLContext c;
	c.context = glXGetCurrentContext();
	return c;
}

void GLContext::present(GLDisplay display, GLDrawable win)
{
	auto swapTime = IG::timeFuncDebug([&](){ glXSwapBuffers(dpy, win.drawable()); }).nSecs();
	if(swapTime > 16000000)
	{
		logWarn("buffer swap took %lldns", (long long)swapTime);
	}
}

void GLContext::present(GLDisplay display, GLDrawable win, GLContext cachedCurrentContext)
{
	present(display, win);
}

void GLContext::finishPresent(GLDisplay display, GLDrawable win) {}

GLContext::operator bool() const
{
	return context;
}

bool GLContext::operator ==(GLContext const &rhs) const
{
	return context == rhs.context;
}

void GLContext::deinit(GLDisplay display)
{
	if(context)
	{
		glXDestroyContext(dpy, context);
		context = nullptr;
	}
}

bool GLContext::bindAPI(API api)
{
	return api == OPENGL_API;
}

void *GLContext::procAddress(const char *funcName)
{
	return (void*)glXGetProcAddress((const GLubyte*)funcName);
}

Base::NativeWindowFormat GLBufferConfig::windowFormat(GLDisplay display)
{
	return fmt;
}

}
