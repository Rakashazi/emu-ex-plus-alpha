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
#include <imagine/base/iphone/IOSWindow.hh>
#include <imagine/base/Error.hh>
#include <imagine/util/UniqueCFObject.hh>
#include <compare>
#include <type_traits>

#ifdef __OBJC__
#import <OpenGLES/EAGL.h>
#import <imagine/base/iphone/EAGLView.hh>
#endif

namespace Base
{

class GLDisplay;
class GLContext;
class GLContextAttributes;

class GLManagerImpl
{
public:
	explicit constexpr operator bool() const { return true; }
	constexpr bool operator ==(GLManagerImpl const&) const = default;
};

class GLDisplayImpl
{
public:
	explicit constexpr operator bool() const { return true; }
	constexpr bool operator ==(GLDisplayImpl const&) const = default;
};

using NativeGLContext = void *; // EAGLContext in ObjC

class IOSGLContext
{
public:
	constexpr IOSGLContext() {}
	IOSGLContext(GLContextAttributes, NativeGLContext shareContext, IG::ErrorCode &);
	operator NativeGLContext() const { return context_.get(); }
	#ifdef __OBJC__
	EAGLContext *context() const { return (__bridge EAGLContext*)context_.get(); }
	#endif
	bool operator ==(IOSGLContext const&) const = default;
	explicit operator bool() const { return (bool)context_; }

protected:
	UniqueCFObject<std::remove_pointer_t<NativeGLContext>> context_{};
};

using NativeGLDrawable = void *; // EAGLView in ObjC

class EAGLViewDrawable
{
public:
	constexpr EAGLViewDrawable() {}
	EAGLViewDrawable(NativeGLDrawable glView);
	operator NativeGLDrawable() const { return glView_.get(); }
	#ifdef __OBJC__
	EAGLView *glView() const { return (__bridge EAGLView*)glView_.get(); }
	#endif
	bool operator ==(EAGLViewDrawable const&) const = default;
	explicit operator bool() const { return (bool)glView_; };

protected:
	UniqueCFObject<std::remove_pointer_t<NativeGLDrawable>> glView_{};
};

struct GLBufferConfig
{
	bool useRGB565 = false;

	Base::NativeWindowFormat windowFormat(Base::ApplicationContext, GLDisplay display) const;
	bool maySupportGLES(GLDisplay, unsigned majorVersion) const;
	constexpr bool operator ==(GLBufferConfig const&) const = default;
};

using GLDrawableImpl = EAGLViewDrawable;
using GLContextImpl = IOSGLContext;
using EAGLViewMakeRenderbufferDelegate = DelegateFunc<IG::Point2D<int>(void *, unsigned int &, unsigned int &)>;
using EAGLViewDeleteRenderbufferDelegate = DelegateFunc<void(unsigned int colorRenderbuffer, unsigned int depthRenderbuffer)>;

extern EAGLViewMakeRenderbufferDelegate makeRenderbuffer;
extern EAGLViewDeleteRenderbufferDelegate deleteRenderbuffer;

}
