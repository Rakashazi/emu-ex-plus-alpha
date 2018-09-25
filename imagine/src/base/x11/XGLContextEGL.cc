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

#define LOGTAG "EGL"
#include <imagine/base/GLContext.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include "internal.hh"

namespace Base
{

// GLDisplay

GLDisplay GLDisplay::getDefault()
{
	return {eglGetDisplay(Config::MACHINE_IS_PANDORA ? EGL_DEFAULT_DISPLAY : (EGLNativeDisplayType)dpy)};
}

// GLContext

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, std::error_code &ec):
	XGLContext{display.eglDisplay(), attr, config, EGL_NO_CONTEXT, ec}
{}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, GLContext shareContext, std::error_code &ec):
	XGLContext{display.eglDisplay(), attr, config, shareContext.nativeObject(), ec}
{}

void GLContext::deinit(GLDisplay display)
{
	EGLContextBase::deinit(display.eglDisplay());
}

void GLContext::setCurrent(GLDisplay display, GLContext context, GLDrawable win)
{
	setCurrentContext(display.eglDisplay(), context.context, win);
}

GLBufferConfig GLContext::makeBufferConfig(GLDisplay display, GLContextAttributes ctxAttr, GLBufferConfigAttributes attr)
{
	auto configResult = chooseConfig(display.eglDisplay(), ctxAttr, attr);
	if(!std::get<bool>(configResult))
	{
		return GLBufferConfig{};
	}
	GLBufferConfig conf{configResult.second};
	#if !defined CONFIG_MACHINE_PANDORA
	{
		// get matching x visual
		EGLint nativeID;
		eglGetConfigAttrib(display.eglDisplay(), conf.glConfig, EGL_NATIVE_VISUAL_ID, &nativeID);
		XVisualInfo viTemplate;
		viTemplate.visualid = nativeID;
		int visuals;
		auto viPtr = XGetVisualInfo(dpy, VisualIDMask, &viTemplate, &visuals);
		if(!viPtr)
		{
			logErr("unable to find matching X Visual");
			return GLBufferConfig{};
		}
		conf.fmt.visual = viPtr->visual;
		conf.fmt.depth = viPtr->depth;
		XFree(viPtr);
	}
	#endif
	return conf;
}

void GLContext::present(GLDisplay display, GLDrawable win)
{
	auto swapTime = IG::timeFuncDebug([&](){ EGLContextBase::swapBuffers(display.eglDisplay(), win); }).nSecs();
	if(swapBuffersIsAsync() && swapTime > 16000000)
	{
		logWarn("buffer swap took %lldns", (long long)swapTime);
	}
}

void GLContext::present(GLDisplay display, GLDrawable win, GLContext cachedCurrentContext)
{
	present(display, win);
}

bool XGLContext::swapBuffersIsAsync()
{
	return !Config::MACHINE_IS_PANDORA;
}

bool GLContext::bindAPI(API api)
{
	if(api == OPENGL_ES_API)
		return eglBindAPI(EGL_OPENGL_ES_API);
	else
		return eglBindAPI(EGL_OPENGL_API);
}

Base::NativeWindowFormat GLBufferConfig::windowFormat(GLDisplay display)
{
	#ifndef CONFIG_MACHINE_PANDORA
	return fmt;
	#else
	return {};
	#endif
}

}
