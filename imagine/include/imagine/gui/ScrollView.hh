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
#include <imagine/gfx/Quads.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/gui/View.hh>

namespace IG::Gfx
{
class RendererCommands;
}

namespace IG
{

class ScrollView : public View
{
public:
	ScrollView(ViewAttachParams attach);
	~ScrollView();
	void onShow() override;
	void onHide() override;
	bool isDoingScrollGesture() const;
	bool isOverScrolled() const;
	int overScroll() const;
	void setScrollOffset(int);
	int scrollOffset() const { return offset; }

protected:
	using VelocityTrackerType = Input::VelocityTracker<float, 1>;

	OnFrameDelegate animate;
	Input::SingleDragTracker<> dragTracker;
	VelocityTrackerType velTracker; // tracks y velocity as pixels/sec
	Gfx::IQuads scrollBarQuads;
	SteadyClockTimePoint lastFrameTimestamp;
	float scrollVel{};
	float scrollAccel{};
	float offsetAsDec{};
	float overScrollVelScale{};
	int offset{};
	int offsetMax{};
	int onDragOffset{};
	int scrollBarYSize{};
	bool contentIsBiggerThanView{};
	bool scrollWholeArea_{};
	bool allowScrollWholeArea_{};

	void setContentSize(WSize size);
	void drawScrollContent(Gfx::RendererCommands &cmds) const;
	bool scrollInputEvent(const Input::MotionEvent &);
	void stopScrollAnimation();
};

}
