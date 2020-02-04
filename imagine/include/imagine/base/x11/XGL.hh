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
#include <imagine/base/EGLContextBase.hh>
#include <imagine/base/WindowConfig.hh>

namespace Base
{

class GLDisplay;
class GLDrawable;

class XGLContext : public EGLContextBase
{
public:
	using EGLContextBase::EGLContextBase;

	constexpr XGLContext() {}
	static void swapPresentedBuffers(Window &win);

protected:
	static bool swapBuffersIsAsync();
};

struct GLBufferConfig : public EGLBufferConfig
{
	#ifndef CONFIG_MACHINE_PANDORA
	NativeWindowFormat fmt;
	#endif

	constexpr GLBufferConfig() {}
	constexpr GLBufferConfig(EGLBufferConfig config):
		EGLBufferConfig{config}
		{}

	explicit operator bool() const
	{
		#ifndef CONFIG_MACHINE_PANDORA
		return fmt.visual;
		#else
		return isInit;
		#endif
	}

	Base::NativeWindowFormat windowFormat(GLDisplay display);
};

using GLDisplayImpl = EGLDisplayConnection;
using GLDrawableImpl = EGLDrawable;

using GLContextImpl = XGLContext;

}

// egl.h normally includes Xlib.h and pollutes the public headers with unwanted symbols,
// clean up common symbols that can clash

#ifndef NO_XLIB_SYMBOL_CLEAN_UP

#undef None
#undef Success

#undef Bool
#undef Status
#undef True
#undef False

#undef QueuedAlready
#undef QueuedAfterReading
#undef QueuedAfterFlush

#undef NoValue
#undef XValue
#undef YValue
#undef WidthValue
#undef HeightValue
#undef AllValues
#undef XNegative
#undef YNegative

#undef USPosition
#undef USSize

#undef PPosition
#undef PSize
#undef PMinSize
#undef PMaxSize
#undef PResizeInc
#undef PAspect
#undef PBaseSize
#undef PWinGravity

#undef InputHint
#undef StateHint
#undef IconPixmapHint
#undef IconWindowHint
#undef IconPositionHint
#undef IconMaskHint
#undef WindowGroupHint
#undef AllHints
#undef XUrgencyHint

#undef WithdrawnState
#undef NormalState
#undef IconicState

#undef DontCareState
#undef ZoomState
#undef InactiveState

#endif
