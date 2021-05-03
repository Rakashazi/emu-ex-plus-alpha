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
#include <imagine/input/DragTracker.hh>
#include <imagine/input/VelocityTracker.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/gui/View.hh>

namespace Gfx
{
class RendererCommands;
}

class ScrollView : public View
{
public:
	ScrollView(ViewAttachParams attach);
	ScrollView(NameString name, ViewAttachParams attach);
	~ScrollView();
	void onShow() override;
	void onHide() override;
	bool isDoingScrollGesture() const;
	bool isOverScrolled() const;
	int overScroll() const;

protected:
	using VelocityTrackerType = Input::VelocityTracker<float, 1>;

	Base::OnFrameDelegate animate;
	Input::SingleDragTracker dragTracker{};
	VelocityTrackerType velTracker{}; // tracks y velocity as pixels/sec
	IG::WindowRect scrollBarRect{};
	IG::FrameTime lastFrameTimestamp{};
	float scrollVel = 0;
	float scrollAccel = 0;
	float offsetAsDec = 0;
	float overScrollVelScale = 0;
	int offset = 0;
	int offsetMax = 0;
	int onDragOffset = 0;
	bool contentIsBiggerThanView = false;
	bool scrollWholeArea_ = false;
	bool allowScrollWholeArea_ = false;

	void setContentSize(IG::WP size);
	void drawScrollContent(Gfx::RendererCommands &cmds);
	bool scrollInputEvent(Input::Event e);
	void setScrollOffset(int o);
	int scrollOffset() const;
	void stopScrollAnimation();
};
