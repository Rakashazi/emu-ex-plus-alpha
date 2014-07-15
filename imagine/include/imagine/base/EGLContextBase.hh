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

#include <EGL/egl.h>

namespace Base
{

class GLConfigAttributes;

class EGLContextBase
{
protected:
	static EGLDisplay display;
	EGLContext context = EGL_NO_CONTEXT;
	EGLSurface dummyPbuff = EGL_NO_SURFACE;
	EGLConfig config{};

	CallResult init(const GLConfigAttributes &attr);
	void deinit();
	static EGLDisplay getDisplay();
	static void setCurrentContext(EGLContextBase *context, Window *win);
	void setCurrentDrawable(Window *win);
	bool isRealCurrentContext();
	void swapBuffers(Window &win);

public:
	constexpr EGLContextBase() {}
	static EGLDisplay eglDisplay();
};

}
