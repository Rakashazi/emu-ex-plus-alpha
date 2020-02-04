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

#include <imagine/config/defs.hh>
#include <imagine/base/iphone/config.h>
#include <imagine/base/Window.hh>

#ifdef __OBJC__
#import <OpenGLES/EAGL.h>
#import <imagine/base/iphone/EAGLView.hh>
#endif

#define CONFIG_GLDRAWABLE_NEEDS_FRAMEBUFFER

namespace Base
{

class GLDisplay;

class GLDisplayImpl {};

class IOSGLContext
{
public:
	constexpr IOSGLContext() {}
	#ifdef __OBJC__
	EAGLContext *context() { return (__bridge EAGLContext*)context_; }
	#endif

protected:
	void *context_{}; // EAGLContext in ObjC
};

class EAGLViewDrawable
{
public:
	constexpr EAGLViewDrawable() {}
	constexpr EAGLViewDrawable(void *glView): glView_{glView} {}
	#ifdef __OBJC__
	EAGLView *glView() { return (__bridge EAGLView*)glView_; }
	#endif
	void *glViewPtr() { return glView_; }

protected:
	void *glView_{}; // EAGLView in ObjC
};

struct GLBufferConfig
{
	bool useRGB565 = false;

	explicit operator bool() const
	{
		return true;
	}

	Base::NativeWindowFormat windowFormat(GLDisplay display);
};

using GLDrawableImpl = EAGLViewDrawable;
using GLContextImpl = IOSGLContext;
using NativeGLContext = void *; // EAGLContext in ObjC
using EAGLViewMakeRenderbufferDelegate = DelegateFunc<IG::Point2D<int>(void *, unsigned int &, unsigned int &)>;
using EAGLViewDeleteRenderbufferDelegate = DelegateFunc<void(unsigned int colorRenderbuffer, unsigned int depthRenderbuffer)>;

extern EAGLViewMakeRenderbufferDelegate makeRenderbuffer;
extern EAGLViewDeleteRenderbufferDelegate deleteRenderbuffer;

}
