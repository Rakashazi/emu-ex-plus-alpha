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
#include <imagine/base/CustomEvent.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/input/Input.hh>
#include <imagine/util/DelegateFunc.hh>
#include <atomic>

namespace Base
{

using namespace IG;

class Window;
class WindowConfig;
class Screen;

class BaseWindow
{
public:
	using SurfaceChange = WindowSurfaceChange;
	using DrawParams = WindowDrawParams;
	using SurfaceChangeDelegate = DelegateFunc<void (Window &win, WindowSurfaceChange change)>;
	using DrawDelegate = DelegateFunc<bool (Window &win, WindowDrawParams params)>;
	using InputEventDelegate = DelegateFunc<bool (Window &win, Input::Event event)>;
	using FocusChangeDelegate = DelegateFunc<void (Window &win, bool in)>;
	using DragDropDelegate = DelegateFunc<void (Window &win, const char *filename)>;
	using DismissRequestDelegate = DelegateFunc<void (Window &win)>;
	using DismissDelegate = DelegateFunc<void (Window &win)>;
	using FreeDelegate = DelegateFunc<void ()>;

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
	void *customDataPtr{};
	std::atomic_bool drawNeeded = false;
	std::atomic_bool notifyDrawAllowed = true;
	// all windows need an initial onSurfaceChange call
	SurfaceChange surfaceChange{SurfaceChange::SURFACE_RESIZED | SurfaceChange::CONTENT_RECT_RESIZED};

	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	Orientation softOrientation_ = VIEW_ROTATE_0;
	Orientation setSoftOrientation = VIEW_ROTATE_0;
	Orientation validSoftOrientations_ = VIEW_ROTATE_0;
	#else
	static constexpr Orientation softOrientation_ = VIEW_ROTATE_0;
	static constexpr Orientation setSoftOrientation = VIEW_ROTATE_0;
	static constexpr Orientation validSoftOrientations_ = VIEW_ROTATE_0;
	#endif

	SurfaceChangeDelegate onSurfaceChange{};
	DrawDelegate onDraw{};
	InputEventDelegate onInputEvent{};
	FocusChangeDelegate onFocusChange{};
	DragDropDelegate onDragDrop{};
	DismissRequestDelegate onDismissRequest{};
	DismissDelegate onDismiss{};
	FreeDelegate onFree{};
	Base::ExitDelegate onExit{};
	Base::ResumeDelegate onResume{};
	Base::CustomEvent drawEvent{"Window::drawEvent"};

	void setOnSurfaceChange(SurfaceChangeDelegate del);
	void setOnDraw(DrawDelegate del);
	void setOnInputEvent(InputEventDelegate del);
	void setOnFocusChange(FocusChangeDelegate del);
	void setOnDragDrop(DragDropDelegate del);
	void setOnDismissRequest(DismissRequestDelegate del);
	void setOnDismiss(DismissDelegate del);
	void setOnFree(FreeDelegate del);
	void init(const WindowConfig &config);
	void initDelegates(const WindowConfig &config);
	void initDefaultValidSoftOrientations();
};

}
