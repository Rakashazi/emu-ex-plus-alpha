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

#ifndef EGL_NO_X11
#define EGL_NO_X11
#endif

#include <imagine/config/defs.hh>
#include <imagine/base/WindowConfig.hh>
#include <imagine/base/Error.hh>
#include <EGL/egl.h>

namespace Base
{

class GLDisplay;
class GLDrawable;
class GLContextAttributes;
class GLBufferConfigAttributes;

struct EGLBufferConfig
{
	EGLConfig glConfig{};

	constexpr EGLBufferConfig() {}
	constexpr EGLBufferConfig(EGLConfig eglConfig):
		glConfig{eglConfig} {}

	Base::NativeWindowFormat windowFormat(GLDisplay display) const;
};

class EGLDisplayConnection
{
public:
	constexpr EGLDisplayConnection() {}
	constexpr EGLDisplayConnection(EGLDisplay display): display{display} {}
	constexpr operator EGLDisplay() const { return display; }
	static IG::ErrorCode initDisplay(EGLDisplay display);
	const char *queryExtensions();
	static const char *errorString(EGLint error);

protected:
	EGLDisplay display{EGL_NO_DISPLAY};
};

class EGLDrawable
{
public:
	constexpr EGLDrawable() {}
	constexpr EGLDrawable(EGLSurface surface): surface{surface} {}
	constexpr operator EGLSurface() const { return surface; }

protected:
	EGLSurface surface{EGL_NO_SURFACE};
};

class EGLContextBase
{
public:
	constexpr EGLContextBase() {}
	EGLContextBase(EGLDisplay display, GLContextAttributes attr, EGLBufferConfig config, EGLContext shareContext, IG::ErrorCode &ec);
	static void swapBuffers(EGLDisplay display, GLDrawable win);

protected:
	EGLContext context{EGL_NO_CONTEXT};

	void deinit(EGLDisplay display);
	static std::pair<bool, EGLConfig> chooseConfig(EGLDisplay display, GLContextAttributes ctxAttr, GLBufferConfigAttributes attr);
	static void setCurrentContext(EGLDisplay display, EGLContext context, GLDrawable win);
};

using NativeGLContext = EGLContext;

}
