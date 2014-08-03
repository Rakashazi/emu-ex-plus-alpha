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
#include <imagine/base/Screen.hh>
#include <imagine/util/DelegateFunc.hh>

#if defined CONFIG_BASE_IOS && defined __ARM_ARCH_6K__
#define CONFIG_GFX_SOFT_ORIENTATION 1
#elif !defined __ANDROID__ && !defined CONFIG_BASE_IOS
#define CONFIG_GFX_SOFT_ORIENTATION 1
#endif

namespace Base
{
using namespace IG;

class Window;

// orientation
static constexpr uint VIEW_ROTATE_0 = bit(0), VIEW_ROTATE_90 = bit(1), VIEW_ROTATE_180 = bit(2), VIEW_ROTATE_270 = bit(3);
static constexpr uint VIEW_ROTATE_AUTO = bit(5);

static const char *orientationToStr(uint o)
{
	using namespace Base;
	switch(o)
	{
		case VIEW_ROTATE_AUTO: return "Auto";
		case VIEW_ROTATE_0: return "0";
		case VIEW_ROTATE_90: return "90";
		case VIEW_ROTATE_180: return "180";
		case VIEW_ROTATE_270: return "270";
		case VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_270: return "0/90/270";
		case VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_180 | VIEW_ROTATE_270: return "0/90/180/270";
		case VIEW_ROTATE_90 | VIEW_ROTATE_270: return "90/270";
		default: bug_branch("%d", o); return 0;
	}
}

#ifdef CONFIG_GFX
static Gfx::GC orientationToGC(uint o)
{
	switch(o)
	{
		case VIEW_ROTATE_0: return Gfx::angleFromDegree(0.);
		case VIEW_ROTATE_90: return Gfx::angleFromDegree(-90.);
		case VIEW_ROTATE_180: return Gfx::angleFromDegree(-180.);
		case VIEW_ROTATE_270: return Gfx::angleFromDegree(90.);
		default: bug_branch("%d", o); return 0.;
	}
}
#endif

static bool orientationIsSideways(uint rotateView)
{
	return rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270;
}

class BaseWindow
{
public:
	struct SurfaceChange
	{
		uint8 flags = 0;
		static constexpr uint8 SURFACE_RESIZED = IG::bit(0),
			CONTENT_RECT_RESIZED = IG::bit(1),
			CUSTOM_VIEWPORT_RESIZED = IG::bit(2);
		static constexpr uint8 RESIZE_BITS =
			SURFACE_RESIZED | CONTENT_RECT_RESIZED | CUSTOM_VIEWPORT_RESIZED;

		constexpr SurfaceChange() {}
		constexpr SurfaceChange(uint8 flags): flags{flags} {}
		bool resized() const
		{
			return flags & RESIZE_BITS;
		}
		bool surfaceResized() const { return flags & SURFACE_RESIZED; }
		bool contentRectResized() const { return flags & CONTENT_RECT_RESIZED; }
		bool customViewportResized() const { return flags & CUSTOM_VIEWPORT_RESIZED; }
		void addSurfaceResized() { flags |= SURFACE_RESIZED; }
		void addContentRectResized() { flags |= CONTENT_RECT_RESIZED; }
		void addCustomViewportResized() { flags |= CUSTOM_VIEWPORT_RESIZED; }
		void removeCustomViewportResized() { unsetBits(flags, CUSTOM_VIEWPORT_RESIZED); }
	};

	struct DrawParams
	{
		FrameTimeBase frameTime_ = 0;
		bool wasResized_ = false;

		constexpr DrawParams() {}
		FrameTimeBase frameTime() const { return frameTime_; }
		bool wasResized() const { return wasResized_; }
	};

	using SurfaceChangeDelegate = DelegateFunc<void (Window &win, SurfaceChange change)>;
	using DrawDelegate = DelegateFunc<void (Window &win, DrawParams params)>;
	using InputEventDelegate = DelegateFunc<void (Window &win, const Input::Event &event)>;
	using FocusChangeDelegate = DelegateFunc<void (Window &win, bool in)>;
	using DragDropDelegate = DelegateFunc<void (Window &win, const char *filename)>;
	using DismissRequestDelegate = DelegateFunc<void (Window &win)>;
	using DismissDelegate = DelegateFunc<void (Window &win)>;

protected:
	int w = 0, h = 0; // size of full window surface
	float wMM = 0, hMM = 0; // size in millimeter
	float mmToPixelXScaler = 0, mmToPixelYScaler = 0;
	#ifdef __ANDROID__
	float wSMM = 0, hSMM = 0; // size in millimeter scaled by OS
	float smmToPixelXScaler = 0, smmToPixelYScaler = 0;
	#endif
	#ifdef CONFIG_BASE_MULTI_SCREEN
	Screen *screen_ = nullptr;
	#endif
	bool drawPosted = false;
	// all windows need an initial onSurfaceChange call
	SurfaceChange surfaceChange{SurfaceChange::SURFACE_RESIZED | SurfaceChange::CONTENT_RECT_RESIZED};

public:
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	uint rotateView = VIEW_ROTATE_0;
	uint preferedOrientation = VIEW_ROTATE_0;
	uint validOrientations = Base::VIEW_ROTATE_0 | Base::VIEW_ROTATE_90 | Base::VIEW_ROTATE_180 | Base::VIEW_ROTATE_270;
	#else
	static constexpr uint rotateView = VIEW_ROTATE_0;
	static constexpr uint preferedOrientation = VIEW_ROTATE_0;
	#endif

protected:
	SurfaceChangeDelegate onSurfaceChange;
	DrawDelegate onDraw;
	InputEventDelegate onInputEvent;
	FocusChangeDelegate onFocusChange;
	DragDropDelegate onDragDrop;
	DismissRequestDelegate onDismissRequest;
	DismissDelegate onDismiss;
};
}
