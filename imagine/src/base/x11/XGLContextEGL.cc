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

namespace Base
{

EGLDisplay EGLContextBase::getDisplay()
{
	return eglGetDisplay(Config::MACHINE_IS_PANDORA ? EGL_DEFAULT_DISPLAY : (EGLNativeDisplayType)dpy);
}

CallResult GLContext::init(const GLConfigAttributes &attr)
{
	auto result = EGLContextBase::init(attr);
	if(result != OK)
		return result;
	// get matching x visual
	#if !defined CONFIG_MACHINE_PANDORA
	{
		EGLint nativeID;
		eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &nativeID);
		XVisualInfo viTemplate;
		viTemplate.visualid = nativeID;
		int visuals;
		vi = XGetVisualInfo(dpy, VisualIDMask, &viTemplate, &visuals);
		if(!vi)
		{
			logErr("unable to find matching X Visual");
			return INVALID_PARAMETER;
		}
	}
	#endif
	return OK;
}


void GLContext::deinit()
{
	EGLContextBase::deinit();
	#if !defined CONFIG_MACHINE_PANDORA
	if(vi)
	{
		XFree(vi);
		vi = nullptr;
	}
	#endif
}

GLConfig GLContext::bufferConfig()
{
	#if defined CONFIG_MACHINE_PANDORA
	return GLConfig{config};
	#else
	return GLConfig{vi, config};
	#endif
}

void GLContext::present(Window &win)
{
	if(swapBuffersIsAsync())
	{
		EGLContextBase::swapBuffers(win);
	}
	else
	{
		glFlush();
		win.presented = true;
	}
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

}
