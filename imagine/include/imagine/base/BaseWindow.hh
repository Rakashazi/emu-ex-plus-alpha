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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/input/Event.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/DelegateFuncSet.hh>
#include <imagine/util/Point2D.hh>
#include <imagine/util/used.hh>
#include <memory>

namespace IG
{

class WindowEvent: public WindowEventVariant, public AddVisit
{
public:
	using WindowEventVariant::WindowEventVariant;
	using AddVisit::visit;
};

class BaseWindow
{
public:
	using SurfaceChange = WindowSurfaceChange;
	using DrawParams = WindowDrawParams;
	using InitDelegate = WindowInitDelegate;

	OnWindowEvent onEvent;

	static constexpr bool shouldRunOnInitAfterAddingWindow = true;

	BaseWindow(ApplicationContext, WindowConfig);
	BaseWindow &operator=(BaseWindow &&) = delete;

protected:
	enum class DrawPhase : uint8_t
	{
		READY,	// Ready to run any onFrame delegates and enter UPDATE phase afterwards
		UPDATE,	// Stay in this phase until drawing is needed, then enter DRAW phase
		DRAW		// Drawing in progress, return to READY phase when finished
	};

	OnExit onExit;
	DelegateFuncSet<OnFrameDelegate> onFrame;
	std::shared_ptr<void> appDataPtr;
	std::shared_ptr<void> rendererDataPtr;
	ConditionalMember<Config::BASE_MULTI_SCREEN, Screen*> screen_{};
	CustomEvent drawEvent;
	WSize winSizePixels{}; // size of full window surface
	F2Size winSizeMM{}; // size in millimeter
	F2Size mmToPixelScaler{};
	 // size in millimeter scaled by OS
	ConditionalMember<Config::envIsAndroid, F2Size> winSizeSMM{};
	ConditionalMember<Config::envIsAndroid, F2Size> smmToPixelScaler{};
	bool drawNeeded{};
	DrawPhase drawPhase{DrawPhase::READY};
	int8_t drawEventPriority_{};
	// all windows need an initial onSurfaceChange call
	WindowSurfaceChangeFlags surfaceChangeFlags{.surfaceResized = true, .contentRectResized = true};
	ConditionalMemberOr<!Config::SYSTEM_ROTATES_WINDOWS, Rotation, Rotation::UP> softOrientation_{Rotation::UP};

	F2Size smmPixelScaler() const;
};

}
