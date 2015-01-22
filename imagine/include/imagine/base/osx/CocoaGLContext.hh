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
#include <imagine/base/Window.hh>

#ifdef __OBJC__
#import <AppKit/NSOpenGL.h>
#endif

namespace Base
{

struct CocoaGLContext
{
protected:
	void *context_{}; // NSOpenGLContext in ObjC

public:
	constexpr CocoaGLContext() {}
	#ifdef __OBJC__
	NSOpenGLContext *context() { return (__bridge NSOpenGLContext*)context_; }
	#endif
};

using GLContextImpl = CocoaGLContext;

}
