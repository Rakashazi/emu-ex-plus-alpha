#pragma once

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

#include <imagine/engine-globals.h>
#include <imagine/base/x11/XWindow.hh>
#ifdef CONFIG_BASE_X11_EGL
#include <imagine/base/EGLContextBase.hh>
#endif

namespace Base
{

#ifdef CONFIG_BASE_X11_EGL

class XGLContext : public EGLContextBase
{
protected:
	#if !defined CONFIG_MACHINE_PANDORA
	XVisualInfo *vi = nullptr;
	#endif
	static bool swapBuffersIsAsync();

public:
	constexpr XGLContext() {}
	void swapPresentedBuffers(Window &win);
};

#else

class GLContext;

class XGLContext
{
protected:
	Display *display = nullptr;
	GLXContext context = nullptr;
	GLXPbuffer dummyPbuff = (GLXPbuffer)0;
	XVisualInfo *vi = nullptr;

	void setCurrentDrawable(Window *win);
	bool isRealCurrentContext();

public:
	constexpr XGLContext() {}
	static void setCurrentContext(XGLContext *context, Window *win);
	void swapPresentedBuffers(Window &win);
};

#endif

using GLContextImpl = XGLContext;

}
