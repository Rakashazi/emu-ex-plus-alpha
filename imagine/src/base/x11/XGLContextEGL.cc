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
	return {eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, xDisplay, nullptr)};
	#endif
}

bool GLDisplay::bindAPI(GL::API api)
{
	if(api == GL::API::OPENGL_ES)
		return eglBindAPI(EGL_OPENGL_ES_API);
	else
		return eglBindAPI(EGL_OPENGL_API);
}

// GLContext

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, IG::ErrorCode &ec):
	EGLContextBase{display, attr, config, EGL_NO_CONTEXT, ec}
{}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, GLContext shareContext, IG::ErrorCode &ec):
	EGLContextBase{display, attr, config, shareContext.nativeObject(), ec}
{}

void GLContext::deinit(GLDisplay display)
{
	EGLContextBase::deinit(display);
}

void GLContext::setCurrent(GLDisplay display, GLContext context, GLDrawable win)
{
	setCurrentContext(display, context.context, win);
}

std::optional<GLBufferConfig> GLContext::makeBufferConfig(GLDisplay display, GLBufferConfigAttributes attr, GL::API api, unsigned majorVersion)
{
	auto renderableType = GLDisplay::makeRenderableType(api, majorVersion);
	return chooseConfig(display, renderableType, attr);
}

void GLContext::present(GLDisplay display, GLDrawable win)
{
	EGLContextBase::swapBuffers(display, win);
}

void GLContext::present(GLDisplay display, GLDrawable win, GLContext cachedCurrentContext)
{
	present(display, win);
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
	auto viPtr = XGetVisualInfo(xDisplay, VisualIDMask, &viTemplate, &visuals);
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
