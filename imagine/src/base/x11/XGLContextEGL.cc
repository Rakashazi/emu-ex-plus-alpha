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

#include <GLES/gl.h> // for glFlush()
#include <imagine/base/GLContext.hh>
#include <imagine/util/time/sys.hh>
#include <imagine/logger/logger.h>
#include "internal.hh"

namespace Base
{

EGLDisplay EGLContextBase::getDisplay()
{
	return eglGetDisplay(Config::MACHINE_IS_PANDORA ? EGL_DEFAULT_DISPLAY : (EGLNativeDisplayType)dpy);
}

CallResult GLContext::init(const GLContextAttributes &attr, const GLBufferConfig &config)
{
	auto result = EGLContextBase::init(attr, config);
	if(result != OK)
		return result;
	return OK;
}

void GLContext::deinit()
{
	EGLContextBase::deinit();
}

void GLContext::setCurrent(GLContext context, Window *win)
{
	setCurrentContext(context.context, win);
}

GLBufferConfig GLContext::makeBufferConfig(const GLContextAttributes &ctxAttr, const GLBufferConfigAttributes &attr)
{
	auto configResult = chooseConfig(ctxAttr, attr);
	if(configResult.first != OK)
	{
		return GLBufferConfig{};
	}
	GLBufferConfig conf{configResult.second};
	#if !defined CONFIG_MACHINE_PANDORA
	{
		// get matching x visual
		EGLint nativeID;
		eglGetConfigAttrib(eglDisplay(), conf.glConfig, EGL_NATIVE_VISUAL_ID, &nativeID);
		XVisualInfo viTemplate;
		viTemplate.visualid = nativeID;
		int visuals;
		auto viPtr = XGetVisualInfo(dpy, VisualIDMask, &viTemplate, &visuals);
		if(!viPtr)
		{
			logErr("unable to find matching X Visual");
			return GLBufferConfig{};
		}
		conf.visual = viPtr->visual;
		conf.depth = viPtr->depth;
		XFree(viPtr);
	}
	#endif
	return conf;
}

void GLContext::present(Window &win)
{
	if(swapBuffersIsAsync())
	{
		auto swapTime = IG::timeFuncDebug([&](){ EGLContextBase::swapBuffers(win); }).toNs();
		if(swapTime > 16000000)
		{
			//logWarn("buffer swap took %lldns", (long long)swapTime);
		}
	}
	else
	{
		glFlush();
		win.presented = true;
	}
}

void GLContext::present(Window &win, GLContext cachedCurrentContext)
{
	present(win);
}

bool XGLContext::swapBuffersIsAsync()
{
	return !Config::MACHINE_IS_PANDORA;
}

void XGLContext::swapPresentedBuffers(Window &win)
{
	if(win.presented)
	{
		assert(!swapBuffersIsAsync()); // shouldn't set presented to true if swap is async
		win.presented = false;
		EGLContextBase::swapBuffers(win);
	}
}

bool GLContext::bindAPI(API api)
{
	if(api == OPENGL_ES_API)
		return eglBindAPI(EGL_OPENGL_ES_API);
	else
		return eglBindAPI(EGL_OPENGL_API);
}

}
