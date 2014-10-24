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
#include <imagine/input/Input.hh>
#include <imagine/util/DelegateFunc.hh>

namespace Config
{
#if defined CONFIG_BASE_IOS && defined __ARM_ARCH_6K__
#define CONFIG_GFX_SOFT_ORIENTATION 1
#elif !defined __ANDROID__ && !defined CONFIG_BASE_IOS
#define CONFIG_GFX_SOFT_ORIENTATION 1
#endif

#if defined CONFIG_GFX_SOFT_ORIENTATION
static constexpr bool SYSTEM_ROTATES_WINDOWS = false;
#else
static constexpr bool SYSTEM_ROTATES_WINDOWS = true;
#endif
}

namespace Base
{
using namespace IG;

class Window;
class WindowConfig;

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
		bool wasResized_ = false;

		constexpr DrawParams() {}
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

	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	uint softOrientation_ = VIEW_ROTATE_0;
	uint setSoftOrientation = VIEW_ROTATE_0;
	uint validSoftOrientations_ = VIEW_ROTATE_0;
	#else
	static constexpr uint softOrientation_ = VIEW_ROTATE_0;
	static constexpr uint setSoftOrientation = VIEW_ROTATE_0;
	static constexpr uint validSoftOrientations_ = VIEW_ROTATE_0;
	#endif

protected:
	SurfaceChangeDelegate onSurfaceChange;
	DrawDelegate onDraw;
	InputEventDelegate onInputEvent;
	FocusChangeDelegate onFocusChange;
	DragDropDelegate onDragDrop;
	DismissRequestDelegate onDismissRequest;
	DismissDelegate onDismiss;

	void setOnSurfaceChange(SurfaceChangeDelegate del);
	void setOnDraw(DrawDelegate del);
	void setOnInputEvent(InputEventDelegate del);
	void setOnFocusChange(FocusChangeDelegate del);
	void setOnDragDrop(DragDropDelegate del);
	void setOnDismissRequest(DismissRequestDelegate del);
	void setOnDismiss(DismissDelegate del);
	void init(const WindowConfig &config);
	void initDelegates(const WindowConfig &config);
	void initDefaultValidSoftOrientations();
};
}
