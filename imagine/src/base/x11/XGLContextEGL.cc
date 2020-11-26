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
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <X11/Xutil.h>

namespace Base
{

// GLDisplay

GLDisplay GLDisplay::getDefault()
{
	#if defined CONFIG_MACHINE_PANDORA
	return {eglGetDisplay(EGL_DEFAULT_DISPLAY)};
	#else
	return {eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, dpy, nullptr)};
	#endif
}

bool GLDisplay::bindAPI(API api)
{
	if(api == API::OPENGL_ES)
		return eglBindAPI(EGL_OPENGL_ES_API);
	else
		return eglBindAPI(EGL_OPENGL_API);
}

// GLContext

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, IG::ErrorCode &ec):
	XGLContext{display, attr, config, EGL_NO_CONTEXT, ec}
{}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, GLContext shareContext, IG::ErrorCode &ec):
	XGLContext{display, attr, config, shareContext.nativeObject(), ec}
{}

void GLContext::deinit(GLDisplay display)
{
	EGLContextBase::deinit(display);
}

void GLContext::setCurrent(GLDisplay display, GLContext context, GLDrawable win)
{
	setCurrentContext(display, context.context, win);
}

std::pair<bool, GLBufferConfig> GLContext::makeBufferConfig(GLDisplay display, GLContextAttributes ctxAttr, GLBufferConfigAttributes attr)
{
	auto [found, eglConfig] = chooseConfig(display, ctxAttr, attr);
	return {found, eglConfig};
}

void GLContext::present(GLDisplay display, GLDrawable win)
{
	auto swapTime = IG::timeFuncDebug([&](){ EGLContextBase::swapBuffers(display, win); });
	if(swapBuffersIsAsync() && swapTime >= IG::Milliseconds(16))
	{
		logWarn("buffer swap took %lldns", (long long)swapTime.count());
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

Base::NativeWindowFormat GLBufferConfig::windowFormat(GLDisplay display) const
{
	if(Config::MACHINE_IS_PANDORA)
		return nullptr;
	// get matching x visual
	EGLint nativeID;
	eglGetConfigAttrib(display, glConfig, EGL_NATIVE_VISUAL_ID, &nativeID);
	XVisualInfo viTemplate{};
	viTemplate.visualid = nativeID;
	int visuals;
	auto viPtr = XGetVisualInfo(dpy, VisualIDMask, &viTemplate, &visuals);
	if(!viPtr)
	{
		logErr("unable to find matching X Visual");
		return nullptr;
	}
	auto visual = viPtr->visual;
	XFree(viPtr);
	return visual;
}

}
