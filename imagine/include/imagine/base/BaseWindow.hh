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
#include <imagine/util/DelegateFuncSet.hh>
#include <imagine/util/Point2D.hh>
#include <imagine/util/typeTraits.hh>
#include <atomic>
#include <memory>

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

protected:
	enum class DrawPhase : uint8_t
	{
		READY,	// Ready to run any onFrame delegates and enter UPDATE phase afterwards
		UPDATE,	// Stay in this phase until drawing is needed, then enter DRAW phase
		DRAW		// Drawing in progress, return to READY phase when finished
	};

	IG_enableMemberIf(Config::BASE_MULTI_SCREEN, Screen *, screen_){};
	std::shared_ptr<void> customDataPtr{};
	SurfaceChangeDelegate onSurfaceChange{};
	DrawDelegate onDraw{};
	InputEventDelegate onInputEvent{};
	FocusChangeDelegate onFocusChange{};
	DragDropDelegate onDragDrop{};
	DismissRequestDelegate onDismissRequest{};
	DismissDelegate onDismiss{};
	Base::OnExit onExit{};
	DelegateFuncSet<Base::OnFrameDelegate> onFrame{};
	Base::CustomEvent drawEvent{"Window::drawEvent"};
	IG::Point2D<int> winSizePixels{}; // size of full window surface
	IG::Point2D<float> winSizeMM{}; // size in millimeter
	IG::Point2D<float> mmToPixelScaler{};
	 // size in millimeter scaled by OS
	IG_enableMemberIf(Config::envIsAndroid, IG::Point2D<float>, winSizeSMM){};
	IG_enableMemberIf(Config::envIsAndroid, IG::Point2D<float>, smmToPixelScaler){};
	std::atomic_bool drawNeeded = false;
	std::atomic<DrawPhase> drawPhase{DrawPhase::READY};
	uint8_t drawEventPriority_{};
	// all windows need an initial onSurfaceChange call
	SurfaceChange surfaceChange{SurfaceChange::SURFACE_RESIZED | SurfaceChange::CONTENT_RECT_RESIZED};
	IG_enableMemberIfOrConstant(!Config::SYSTEM_ROTATES_WINDOWS, Orientation, VIEW_ROTATE_0, softOrientation_){VIEW_ROTATE_0};
	IG_enableMemberIfOrConstant(!Config::SYSTEM_ROTATES_WINDOWS, Orientation, VIEW_ROTATE_0, setSoftOrientation){VIEW_ROTATE_0};
	IG_enableMemberIfOrConstant(!Config::SYSTEM_ROTATES_WINDOWS, Orientation, VIEW_ROTATE_0, validSoftOrientations_){VIEW_ROTATE_0};

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
	IG::Point2D<float> smmPixelScaler() const;
};

}
