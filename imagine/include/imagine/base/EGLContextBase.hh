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
#include <utility>

namespace Base
{

class GLContextAttributes;
class GLBufferConfigAttributes;

class EGLContextBase
{
protected:
	EGLContext context = EGL_NO_CONTEXT;

	CallResult init(const GLContextAttributes &attr, const GLBufferConfig &config);
	void deinit();
	static std::pair<CallResult, EGLConfig> chooseConfig(const GLContextAttributes &ctxAttr, const GLBufferConfigAttributes &attr);
	static EGLDisplay getDisplay();
	static void setCurrentContext(EGLContext context, Window *win);
	static void swapBuffers(Window &win);

public:
	constexpr EGLContextBase() {}
	static EGLDisplay eglDisplay();
};

}
